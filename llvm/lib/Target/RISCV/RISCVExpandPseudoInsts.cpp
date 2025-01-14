//===-- RISCVExpandPseudoInsts.cpp - Expand pseudo instructions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVBaseInfo.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define RISCV_EXPAND_PSEUDO_NAME "RISCV pseudo instruction expansion pass"

namespace {

class RISCVExpandPseudo : public MachineFunctionPass {
public:
  const RISCVInstrInfo *TII;
  static char ID;
  CHERIoTImportedFunctionSet &ImportedFunctions;

  RISCVExpandPseudo(CHERIoTImportedFunctionSet &CHERIoTImports)
      : MachineFunctionPass(ID), ImportedFunctions(CHERIoTImports) {
    initializeRISCVExpandPseudoPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return RISCV_EXPAND_PSEUDO_NAME; }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAuipcInstPair(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI,
                           MachineBasicBlock::iterator &NextMBBI,
                           unsigned FlagsHi, unsigned SecondOpcode);
  bool expandLoadLocalAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadAddress(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI,
                         MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSIEAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSGDAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);

  bool expandAuipccInstPair(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            MachineBasicBlock::iterator &NextMBBI,
                            unsigned FlagsHi, unsigned SecondOpcode,
                            bool InBounds = false);
  bool expandAuicgpInstPair(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            MachineBasicBlock::iterator &NextMBBI,
                            unsigned SecondOpcode, bool InBounds = false);
  bool expandCapLoadLocalCap(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI,
                             bool InBounds);
  bool expandCapLoadGlobalCap(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandCapLoadTLSIEAddress(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI);
  bool expandCapLoadTLSGDCap(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI);
  bool expandVSetVL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool expandVMSET_VMCLR(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI, unsigned Opcode);
  bool expandVSPILL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool expandVRELOAD(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  /// Expand a CHERIoT cross-compartment call into a call to the switcher using
  /// an import-table entry.
  bool expandCompartmentCall(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI);
  /// Expand a CHERIoT cross-library call into a call via an import-table entry.
  bool expandLibraryCall(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI,
                         MachineBasicBlock::iterator &NextMBBI);
  /**
   * Helper that inserts a load of the import table for `Fn` at `MBBI`.  This
   * inserts an AUIPCC and so will split `MBB`.  Calls the result if
   * `CallImportTarget` is true. `TreatAsLibrary` should be set if the exported
   * function is / may be exported from this compartment but, at this call site,
   * should be treated as a library call.
   */
  MachineBasicBlock *insertLoadOfImportTable(MachineBasicBlock &MBB,
                                             MachineBasicBlock::iterator MBBI,
                                             const Function *Fn,
                                             Register DestReg,
                                             bool TreatAsLibrary = false,
                                             bool CallImportTarget = false);
  /**
   * Helper that inserts a load from the import table identified by an import
   * and export table entry symbol.
   *
   * Calls the result if `CallImportTarget` is true.
   */
  MachineBasicBlock *insertLoadOfImportTable(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
      MCSymbol *ImportSymbol, MCSymbol *ExportSymbol, Register DestReg,
      bool IsLibrary, bool IsPublic, bool CallImportTarget);
};

char RISCVExpandPseudo::ID = 0;

bool RISCVExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  TII = static_cast<const RISCVInstrInfo *>(MF.getSubtarget().getInstrInfo());
  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);
  return Modified;
}

bool RISCVExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RISCVExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  bool InBounds = true;
  // RISCVInstrInfo::getInstSizeInBytes hard-codes the number of expanded
  // instructions for each pseudo, and must be updated when adding new pseudos
  // or changing existing ones.
  switch (MBBI->getOpcode()) {
  case RISCV::PseudoLLA:
    return expandLoadLocalAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA:
    return expandLoadAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_IE:
    return expandLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_GD:
    return expandLoadTLSGDAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoCLLC:
    InBounds = false;
    LLVM_FALLTHROUGH;
  case RISCV::PseudoCLLCInbounds:
    return expandCapLoadLocalCap(MBB, MBBI, NextMBBI, InBounds);
  case RISCV::PseudoCLGC:
    return expandCapLoadGlobalCap(MBB, MBBI, NextMBBI);
  case RISCV::PseudoCLA_TLS_IE:
    return expandCapLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoCLC_TLS_GD:
    return expandCapLoadTLSGDCap(MBB, MBBI, NextMBBI);
  case RISCV::PseudoVSETVLI:
  case RISCV::PseudoVSETIVLI:
    return expandVSetVL(MBB, MBBI);
  case RISCV::PseudoVMCLR_M_B1:
  case RISCV::PseudoVMCLR_M_B2:
  case RISCV::PseudoVMCLR_M_B4:
  case RISCV::PseudoVMCLR_M_B8:
  case RISCV::PseudoVMCLR_M_B16:
  case RISCV::PseudoVMCLR_M_B32:
  case RISCV::PseudoVMCLR_M_B64:
    // vmclr.m vd => vmxor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXOR_MM);
  case RISCV::PseudoVMSET_M_B1:
  case RISCV::PseudoVMSET_M_B2:
  case RISCV::PseudoVMSET_M_B4:
  case RISCV::PseudoVMSET_M_B8:
  case RISCV::PseudoVMSET_M_B16:
  case RISCV::PseudoVMSET_M_B32:
  case RISCV::PseudoVMSET_M_B64:
    // vmset.m vd => vmxnor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXNOR_MM);
  case RISCV::PseudoVSPILL2_M1:
  case RISCV::PseudoVSPILL2_M2:
  case RISCV::PseudoVSPILL2_M4:
  case RISCV::PseudoVSPILL3_M1:
  case RISCV::PseudoVSPILL3_M2:
  case RISCV::PseudoVSPILL4_M1:
  case RISCV::PseudoVSPILL4_M2:
  case RISCV::PseudoVSPILL5_M1:
  case RISCV::PseudoVSPILL6_M1:
  case RISCV::PseudoVSPILL7_M1:
  case RISCV::PseudoVSPILL8_M1:
    return expandVSPILL(MBB, MBBI);
  case RISCV::PseudoVRELOAD2_M1:
  case RISCV::PseudoVRELOAD2_M2:
  case RISCV::PseudoVRELOAD2_M4:
  case RISCV::PseudoVRELOAD3_M1:
  case RISCV::PseudoVRELOAD3_M2:
  case RISCV::PseudoVRELOAD4_M1:
  case RISCV::PseudoVRELOAD4_M2:
  case RISCV::PseudoVRELOAD5_M1:
  case RISCV::PseudoVRELOAD6_M1:
  case RISCV::PseudoVRELOAD7_M1:
  case RISCV::PseudoVRELOAD8_M1:
    return expandVRELOAD(MBB, MBBI);
  case RISCV::PseudoCompartmentCall:
    return expandCompartmentCall(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLibraryCall:
    return expandLibraryCall(MBB, MBBI, NextMBBI);
  }

  return false;
}

MachineBasicBlock *RISCVExpandPseudo::insertLoadOfImportTable(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    const Function *Fn, Register DestReg, bool TreatAsLibrary,
    bool CallImportTarget) {
  auto *MF = MBB.getParent();
  const StringRef ImportName = Fn->getName();
  // We can hit this code path if we need to do a library-style import
  // for a local exported function.
  const bool IsLibrary = Fn->getCallingConv() == CallingConv::CHERI_LibCall;
  const StringRef CompartmentName =
      IsLibrary ? "libcalls"
                : Fn->getFnAttribute("cheri-compartment").getValueAsString();
  // Is this actually a compartment call that is locally imported?

  auto ImportEntryName = getImportExportTableName(
      CompartmentName, ImportName,
      TreatAsLibrary ? CallingConv::CHERI_LibCall : Fn->getCallingConv(),
      /*IsImport*/ true);
  // If this isn't really a library call then the export symbol will be
  // different.
  auto ExportEntryName = getImportExportTableName(CompartmentName, ImportName,
                                                  Fn->getCallingConv(),
                                                  /*IsImport*/ false);
  // Create the symbol for the import entry.  We don't use this symbol
  // directly (yet) but we need to allocate storage for the string where
  // it will remain valid for the duration of codegen.
  MCSymbol *ImportSymbol = MF->getContext().getOrCreateSymbol(ImportEntryName);
  MCSymbol *ExportSymbol = MF->getContext().getOrCreateSymbol(ExportEntryName);
  return insertLoadOfImportTable(MBB, MBBI, ImportSymbol, ExportSymbol, DestReg,
                                 IsLibrary || TreatAsLibrary,
                                 Fn->hasExternalLinkage(), CallImportTarget);
}

MachineBasicBlock *RISCVExpandPseudo::insertLoadOfImportTable(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MCSymbol *ImportSymbol, MCSymbol *ExportSymbol, Register DestReg,
    bool IsLibrary, bool IsPublic, bool CallImportTarget) {
  auto *MF = MBB.getParent();
  const DebugLoc DL = MBBI->getDebugLoc();
  MachineBasicBlock *NewMBB =
      MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this
  // label to be emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), DestReg)
      .addExternalSymbol(ImportSymbol->getName().data(),
                         RISCVII::MO_CHERIOT_COMPARTMENT_HI);
  BuildMI(NewMBB, DL, TII->get(RISCV::CLC_64), DestReg)
      .addReg(DestReg, RegState::Kill)
      .addMBB(NewMBB, RISCVII::MO_CHERIOT_COMPARTMENT_LO_I);

  if (CallImportTarget)
    BuildMI(NewMBB, DL, TII->get(RISCV::C_CJALR))
        .addReg(DestReg, RegState::Kill);

  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  ImportedFunctions.insert(
      {ImportSymbol->getName(), ExportSymbol->getName(), IsLibrary, IsPublic});
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);
  return NewMBB;
}

bool RISCVExpandPseudo::expandCompartmentCall(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {

  const MachineOperand Callee = MBBI->getOperand(0);
  MachineInstr &MI = *MBBI;
  const DebugLoc DL = MBBI->getDebugLoc();
  auto *MF = MBB.getParent();
  if (Callee.isGlobal()) {
    // If this is a global, check if it's in the same compartment.  If so, we
    // want to lower as a direct ccall.
    auto *Fn = cast<Function>(Callee.getGlobal());
    if (MF->getFunction().hasFnAttribute("cheri-compartment") &&
        (Fn->getFnAttribute("cheri-compartment").getValueAsString() ==
         MF->getFunction()
             .getFnAttribute("cheri-compartment")
             .getValueAsString())) {
      if (isSafeToDirectCall(MF->getFunction(), *Fn)) {
        MI.setDesc(TII->get(RISCV::PseudoCCALL));
        return true;
      }
      // If this is within the same compartment but must be called via an import
      // table entry, then expand it as a library call.
      return expandLibraryCall(MBB, MBBI, NextMBBI);
    }
  }
  // Expand a cross-compartment call into a auipcc + clc to get the compartment
  // switcher, followed by a compressed cjalr to invoke it.  This is running
  // post-RA, but the ABI guarantees that C7 is not required to be preserved
  // here and so we can use it.
  // FIXME: This copies and pastes a lot of code from expandAuipccInstPair.

  const MachineOperand Switcher =
      MachineOperand::CreateES(".compartment_switcher", 0);

  auto *NewMBB = MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), RISCV::C7)
      .addDisp(Switcher, 0, RISCVII::MO_CHERIOT_COMPARTMENT_HI);
  BuildMI(NewMBB, DL, TII->get(RISCV::CLC_64), RISCV::C7)
      .addReg(RISCV::C7, RegState::Kill)
      .addMBB(NewMBB, RISCVII::MO_CHERIOT_COMPARTMENT_LO_I);
  BuildMI(NewMBB, DL, TII->get(RISCV::C_CJALR))
      .addReg(RISCV::C7, RegState::Kill);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  if (Callee.isGlobal()) {
    auto *Fn = cast<Function>(Callee.getGlobal());
    insertLoadOfImportTable(MBB, MBBI, Fn, RISCV::C6);
  } else {
    assert(Callee.isReg() && "Expected register operand");
    if (Callee.getReg() != RISCV::C6) {
      BuildMI(&MBB, DL, TII->get(RISCV::CMove)).addReg(RISCV::C6).add(Callee);
    }
  }

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandLibraryCall(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Expand a cross-library call into a auipcc + clc to get the import table
  // entry , followed by a compressed cjalr to invoke it.  This is running
  // post-RA, but the ABI guarantees that C7 is not required to be preserved
  // here and so we can use it.
  const MachineOperand Callee = MBBI->getOperand(0);
  MachineInstr &MI = *MBBI;
  auto *MF = MBB.getParent();
  if (Callee.isGlobal()) {
    auto *Fn = cast<Function>(Callee.getGlobal());
    // If this is a global, check if it's defined in the same module and has a
    // compatible interrupt status.  If so, we want to lower as a direct ccall.
    if (!Fn->isDeclaration() && isSafeToDirectCall(MF->getFunction(), *Fn)) {
      MI.setDesc(TII->get(RISCV::PseudoCCALL));
      return true;
    }
    insertLoadOfImportTable(MBB, MBBI, Fn, RISCV::C7, true, true);

    NextMBBI = MBB.end();
  } else if (Callee.isSymbol()) {
    // We can see symbols here from calls to runtime functions that are
    // inserted by the back end, for example memcpy expanded from the LLVM
    // intrinsic.  These don't have accompanying LLVM functions and so we just
    // need to treat them as an external library call.
    auto ImportEntryName = getImportExportTableName(
        "libcalls", Callee.getSymbolName(), CallingConv::CHERI_LibCall,
        /*IsImport*/ true);
    auto ExportEntryName = getImportExportTableName(
        "libcalls", Callee.getSymbolName(), CallingConv::CHERI_LibCall,
        /*IsImport*/ false);
    // Create the symbol for the import entry.  We don't use this symbol
    // directly (yet) but we need to allocate storage for the string where
    // it will remain valid for the duration of codegen.
    MCSymbol *ImportSymbol =
        MF->getContext().getOrCreateSymbol(ImportEntryName);
    MCSymbol *ExportSymbol =
        MF->getContext().getOrCreateSymbol(ExportEntryName);
    insertLoadOfImportTable(MBB, MBBI, ImportSymbol, ExportSymbol, RISCV::C7,
                            true, true, true);

    NextMBBI = MBB.end();
  } else {
    assert(Callee.isReg() && "Expected register operand");
    // Indirect library calls are just cjalr instructions.
    BuildMI(&MBB, MI.getDebugLoc(), TII->get(RISCV::C_CJALR)).add(Callee);
  }
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandAuicgpInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned SecondOpcode,
    bool InBounds) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  auto *MF = MBB.getParent();

  bool HasTmpReg = MI.getNumOperands() > 2;
  Register DestReg = MI.getOperand(0).getReg();
  Register TmpReg = MI.getOperand(HasTmpReg ? 1 : 0).getReg();
  const MachineOperand &Symbol = MI.getOperand(HasTmpReg ? 2 : 1);

  auto *NewMBB = MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUICGP), DestReg)
      .addDisp(Symbol, 0, RISCVII::MO_CHERIOT_COMPARTMENT_HI);
  BuildMI(NewMBB, DL, TII->get(SecondOpcode), DestReg)
      .addReg(DestReg, RegState::Kill)
      .addMBB(NewMBB, RISCVII::MO_CHERIOT_COMPARTMENT_LO_I);

  if (!InBounds)
    BuildMI(NewMBB, DL, TII->get(RISCV::CSetBoundsImm), DestReg)
        .addReg(DestReg, RegState::Kill)
        .addDisp(Symbol, 0, RISCVII::MO_CHERIOT_COMPARTMENT_SIZE);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandAuipcInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  const MachineOperand &Symbol = MI.getOperand(1);

  MachineBasicBlock *NewMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPC), DestReg)
      .addDisp(Symbol, 0, FlagsHi);
  BuildMI(NewMBB, DL, TII->get(SecondOpcode), DestReg)
      .addReg(DestReg)
      .addMBB(NewMBB, RISCVII::MO_PCREL_LO);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandLoadLocalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI,
                             RISCV::ADDI);
}

bool RISCVExpandPseudo::expandLoadAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  unsigned SecondOpcode;
  unsigned FlagsHi;
  if (MF->getTarget().isPositionIndependent()) {
    const auto &STI = MF->getSubtarget<RISCVSubtarget>();
    SecondOpcode = STI.is64Bit() ? RISCV::LD : RISCV::LW;
    FlagsHi = RISCVII::MO_GOT_HI;
  } else {
    SecondOpcode = RISCV::ADDI;
    FlagsHi = RISCVII::MO_PCREL_HI;
  }
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, FlagsHi, SecondOpcode);
}

bool RISCVExpandPseudo::expandLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::LD : RISCV::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GOT_HI,
                             SecondOpcode);
}

bool RISCVExpandPseudo::expandLoadTLSGDAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GD_HI,
                             RISCV::ADDI);
}

bool RISCVExpandPseudo::expandAuipccInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode, bool InBounds) {
  bool IsCheriot =
      MBB.getParent()->getSubtarget<RISCVSubtarget>().getTargetABI() ==
      RISCVABI::ABI_CHERIOT;
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  bool HasTmpReg = MI.getNumOperands() > 2;
  Register DestReg = MI.getOperand(0).getReg();
  Register TmpReg = MI.getOperand(HasTmpReg ? 1 : 0).getReg();
  const MachineOperand &Symbol = MI.getOperand(HasTmpReg ? 2 : 1);

  MachineBasicBlock *NewMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), TmpReg)
      .addDisp(Symbol, 0, FlagsHi);
  BuildMI(NewMBB, DL, TII->get(SecondOpcode), DestReg)
      .addReg(TmpReg)
      .addMBB(NewMBB, IsCheriot ? RISCVII::MO_CHERIOT_COMPARTMENT_LO_I
                                : RISCVII::MO_PCREL_LO);
  if (!InBounds && MF->getSubtarget<RISCVSubtarget>().isRV32E() &&
      Symbol.isGlobal() && isa<GlobalVariable>(Symbol.getGlobal()) &&
      (cast<GlobalVariable>(Symbol.getGlobal())->getSection() !=
       ".compartment_imports"))
    BuildMI(NewMBB, DL, TII->get(RISCV::CSetBoundsImm), DestReg)
        .addReg(DestReg)
        .addDisp(Symbol, 0, RISCVII::MO_CHERIOT_COMPARTMENT_SIZE);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandCapLoadLocalCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, bool InBounds) {
  if (MBB.getParent()->getSubtarget<RISCVSubtarget>().getTargetABI() ==
      RISCVABI::ABI_CHERIOT) {
    const DebugLoc DL = MBBI->getDebugLoc();
    const MachineOperand &Symbol = MBBI->getOperand(1);
    const GlobalValue *GV = Symbol.getGlobal();
    if (isa<Function>(GV) || cast<GlobalVariable>(GV)->isConstant()) {
      if (auto *Fn = dyn_cast<Function>(GV)) {
        auto CC = Fn->getCallingConv();
        if ((getInterruptStatus(*Fn) != Interrupts::Inherit) ||
            (CC == CallingConv::CHERI_CCall) ||
            (CC == CallingConv::CHERI_CCallee)) {
          insertLoadOfImportTable(MBB, MBBI, Fn, MBBI->getOperand(0).getReg());
          NextMBBI = MBB.end();
          MBBI->eraseFromParent();
          return true;
        }
      }
      return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                                  RISCVII::MO_CHERIOT_COMPARTMENT_HI,
                                  RISCV::CIncOffsetImm, InBounds);
    }
    return expandAuicgpInstPair(MBB, MBBI, NextMBBI, RISCV::CIncOffsetImm,
                                InBounds);
  }
  return expandAuipccInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI, RISCV::CIncOffsetImm);
}

bool RISCVExpandPseudo::expandCapLoadGlobalCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::CLC_128 : RISCV::CLC_64;
  return expandAuipccInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_CAPTAB_PCREL_HI,
                              SecondOpcode);
}

bool RISCVExpandPseudo::expandCapLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::CLD : RISCV::CLW;
  return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                              RISCVII::MO_TLS_IE_CAPTAB_PCREL_HI,
                              SecondOpcode);
}

bool RISCVExpandPseudo::expandCapLoadTLSGDCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                              RISCVII::MO_TLS_GD_CAPTAB_PCREL_HI,
                              RISCV::CIncOffsetImm);
}

bool RISCVExpandPseudo::expandVSetVL(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MBBI) {
  assert(MBBI->getNumExplicitOperands() == 3 && MBBI->getNumOperands() >= 5 &&
         "Unexpected instruction format");

  DebugLoc DL = MBBI->getDebugLoc();

  assert((MBBI->getOpcode() == RISCV::PseudoVSETVLI ||
          MBBI->getOpcode() == RISCV::PseudoVSETIVLI) &&
         "Unexpected pseudo instruction");
  unsigned Opcode;
  if (MBBI->getOpcode() == RISCV::PseudoVSETVLI)
    Opcode = RISCV::VSETVLI;
  else
    Opcode = RISCV::VSETIVLI;
  const MCInstrDesc &Desc = TII->get(Opcode);
  assert(Desc.getNumOperands() == 3 && "Unexpected instruction format");

  Register DstReg = MBBI->getOperand(0).getReg();
  bool DstIsDead = MBBI->getOperand(0).isDead();
  BuildMI(MBB, MBBI, DL, Desc)
      .addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
      .add(MBBI->getOperand(1))  // VL
      .add(MBBI->getOperand(2)); // VType

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandVMSET_VMCLR(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          unsigned Opcode) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register DstReg = MBBI->getOperand(0).getReg();
  const MCInstrDesc &Desc = TII->get(Opcode);
  BuildMI(MBB, MBBI, DL, Desc, DstReg)
      .addReg(DstReg, RegState::Undef)
      .addReg(DstReg, RegState::Undef);
  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandVSPILL(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MBBI) {
  const TargetRegisterInfo *TRI =
      MBB.getParent()->getSubtarget().getRegisterInfo();
  DebugLoc DL = MBBI->getDebugLoc();
  Register SrcReg = MBBI->getOperand(0).getReg();
  Register Base = MBBI->getOperand(1).getReg();
  Register VL = MBBI->getOperand(2).getReg();
  auto ZvlssegInfo = TII->isRVVSpillForZvlsseg(MBBI->getOpcode());
  if (!ZvlssegInfo)
    return false;
  unsigned NF = ZvlssegInfo->first;
  unsigned LMUL = ZvlssegInfo->second;
  assert(NF * LMUL <= 8 && "Invalid NF/LMUL combinations.");
  unsigned Opcode = RISCV::VS1R_V;
  unsigned SubRegIdx = RISCV::sub_vrm1_0;
  static_assert(RISCV::sub_vrm1_7 == RISCV::sub_vrm1_0 + 7,
                "Unexpected subreg numbering");
  if (LMUL == 2) {
    Opcode = RISCV::VS2R_V;
    SubRegIdx = RISCV::sub_vrm2_0;
    static_assert(RISCV::sub_vrm2_3 == RISCV::sub_vrm2_0 + 3,
                  "Unexpected subreg numbering");
  } else if (LMUL == 4) {
    Opcode = RISCV::VS4R_V;
    SubRegIdx = RISCV::sub_vrm4_0;
    static_assert(RISCV::sub_vrm4_1 == RISCV::sub_vrm4_0 + 1,
                  "Unexpected subreg numbering");
  } else
    assert(LMUL == 1 && "LMUL must be 1, 2, or 4.");

  for (unsigned I = 0; I < NF; ++I) {
    BuildMI(MBB, MBBI, DL, TII->get(Opcode))
        .addReg(TRI->getSubReg(SrcReg, SubRegIdx + I))
        .addReg(Base)
        .addMemOperand(*(MBBI->memoperands_begin()));
    if (I != NF - 1)
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADD), Base)
          .addReg(Base)
          .addReg(VL);
  }
  MBBI->eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandVRELOAD(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator MBBI) {
  const TargetRegisterInfo *TRI =
      MBB.getParent()->getSubtarget().getRegisterInfo();
  DebugLoc DL = MBBI->getDebugLoc();
  Register DestReg = MBBI->getOperand(0).getReg();
  Register Base = MBBI->getOperand(1).getReg();
  Register VL = MBBI->getOperand(2).getReg();
  auto ZvlssegInfo = TII->isRVVSpillForZvlsseg(MBBI->getOpcode());
  if (!ZvlssegInfo)
    return false;
  unsigned NF = ZvlssegInfo->first;
  unsigned LMUL = ZvlssegInfo->second;
  assert(NF * LMUL <= 8 && "Invalid NF/LMUL combinations.");
  unsigned Opcode = RISCV::VL1RE8_V;
  unsigned SubRegIdx = RISCV::sub_vrm1_0;
  static_assert(RISCV::sub_vrm1_7 == RISCV::sub_vrm1_0 + 7,
                "Unexpected subreg numbering");
  if (LMUL == 2) {
    Opcode = RISCV::VL2RE8_V;
    SubRegIdx = RISCV::sub_vrm2_0;
    static_assert(RISCV::sub_vrm2_3 == RISCV::sub_vrm2_0 + 3,
                  "Unexpected subreg numbering");
  } else if (LMUL == 4) {
    Opcode = RISCV::VL4RE8_V;
    SubRegIdx = RISCV::sub_vrm4_0;
    static_assert(RISCV::sub_vrm4_1 == RISCV::sub_vrm4_0 + 1,
                  "Unexpected subreg numbering");
  } else
    assert(LMUL == 1 && "LMUL must be 1, 2, or 4.");

  for (unsigned I = 0; I < NF; ++I) {
    BuildMI(MBB, MBBI, DL, TII->get(Opcode),
            TRI->getSubReg(DestReg, SubRegIdx + I))
        .addReg(Base)
        .addMemOperand(*(MBBI->memoperands_begin()));
    if (I != NF - 1)
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADD), Base)
          .addReg(Base)
          .addReg(VL);
  }
  MBBI->eraseFromParent();
  return true;
}

} // end of anonymous namespace

namespace llvm {

template <> Pass *callDefaultCtor<RISCVExpandPseudo>() {
  static CHERIoTImportedFunctionSet CHERIoTImports;
  return new RISCVExpandPseudo(CHERIoTImports);
}

FunctionPass *
createRISCVExpandPseudoPass(CHERIoTImportedFunctionSet &CHERIoTImports) {
  return new RISCVExpandPseudo(CHERIoTImports);
}

} // end of namespace llvm

INITIALIZE_PASS(RISCVExpandPseudo, "riscv-expand-pseudo",
                RISCV_EXPAND_PSEUDO_NAME, false, false)
