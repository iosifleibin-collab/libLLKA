// vim: set sw=4 ts=4 sts=4 expandtab :

#include <llka_structure.h>
#include <llka_nucleotide.h>

#include "extend.h"
#include "nucleotide.hpp"
#include "structure_util.hpp"
#include "util/elementaries.h"

#include <cassert>
#include <cstring>

void LLKA_CC LLKA_appendAtom(const LLKA_Atom *atom, LLKA_Structure *stru)
{
    auto newAtoms = new LLKA_Atom[stru->nAtoms + 1];
    if (stru->nAtoms > 0)
        std::memcpy(newAtoms, stru->atoms, stru->nAtoms * sizeof(LLKA_Atom));

    LLKA_duplicateAtom(atom, &newAtoms[stru->nAtoms]);
    delete[] stru->atoms;
    stru->atoms = newAtoms;
    stru->nAtoms++;
}

LLKA_RetCode LLKA_CC LLKA_baseKind(const char *compId, LLKA_BaseKind *kind)
{
    if (LLKAInternal::findBaseKind(compId, *kind))
        return LLKA_OK;

    return LLKA_E_INVALID_ARGUMENT;
}

void LLKA_CC LLKA_destroyAtom(const LLKA_Atom *atom)
{
    LLKAInternal::destroyString(atom->type_symbol);
    LLKAInternal::destroyString(atom->label_atom_id);
    LLKAInternal::destroyString(atom->label_entity_id);
    LLKAInternal::destroyString(atom->label_comp_id);
    LLKAInternal::destroyString(atom->label_asym_id);
    LLKAInternal::destroyString(atom->auth_atom_id);
    LLKAInternal::destroyString(atom->auth_comp_id);
    LLKAInternal::destroyString(atom->auth_asym_id);
    LLKAInternal::destroyString(atom->pdbx_PDB_ins_code);
}

void LLKA_CC LLKA_destroyStructure(const LLKA_Structure *stru)
{
    for (size_t idx = 0; idx < stru->nAtoms; idx++)
        LLKA_destroyAtom(&stru->atoms[idx]);

    delete [] stru->atoms;
}

void LLKA_CC LLKA_destroyStructureView(const LLKA_StructureView *view)
{
    delete [] view->atoms;
}

void LLKA_CC LLKA_destroyStructures(const LLKA_Structures *strus)
{
    for (size_t idx = 0; idx < strus->nStrus; idx++)
        LLKA_destroyStructure(&strus->strus[idx]);

    delete [] strus->strus;
}

void LLKA_CC LLKA_duplicateAtom(const LLKA_Atom *source, LLKA_Atom *target)
{
    target->id = source->id;
    target->type_symbol = LLKAInternal::duplicateString(source->type_symbol);
    target->label_entity_id = LLKAInternal::duplicateString(source->label_entity_id);
    target->label_atom_id = LLKAInternal::duplicateString(source->label_atom_id);
    target->label_comp_id = LLKAInternal::duplicateString(source->label_comp_id);
    target->label_asym_id = LLKAInternal::duplicateString(source->label_asym_id);
    target->auth_atom_id = LLKAInternal::duplicateString(source->auth_atom_id);
    target->auth_comp_id = LLKAInternal::duplicateString(source->auth_comp_id);
    target->auth_asym_id = LLKAInternal::duplicateString(source->auth_asym_id);
    target->coords = source->coords;
    target->label_seq_id = source->label_seq_id;
    target->auth_seq_id = source->auth_seq_id;
    target->pdbx_PDB_ins_code = LLKAInternal::duplicateString(source->pdbx_PDB_ins_code);
    target->pdbx_PDB_model_num = source->pdbx_PDB_model_num;
    target->label_alt_id = source->label_alt_id;
}

void LLKA_CC LLKA_duplicateStructure(const LLKA_Structure *source, LLKA_Structure *target)
{
    target->atoms = nullptr;
    target->nAtoms = source->nAtoms;

    if (target->nAtoms < 1)
        return;

    target->atoms = new LLKA_Atom[target->nAtoms];
    for (size_t idx = 0; idx < target->nAtoms; idx++)
        LLKA_duplicateAtom(&source->atoms[idx], &target->atoms[idx]);
}

void LLKA_CC LLKA_initAtom(
    uint32_t id,
    const char *type_symbol,
    const char *label_atom_id,
    const char *label_entity_id,
    const char *label_comp_id,
    const char *label_asym_id,
    const char *auth_atom_id,
    const char *auth_comp_id,
    const char *auth_asym_id,
    int32_t label_seq_id,
    char label_alt_id,
    int32_t auth_seq_id,
    const char *pdbx_PDB_ins_code,
    int32_t pdbx_PDB_model_num,
    const LLKA_Point *coords,
    LLKA_Atom *atom
)
{
    atom->id = id;
    atom->type_symbol = LLKAInternal::duplicateString(type_symbol);
    atom->label_atom_id = LLKAInternal::duplicateString(label_atom_id);
    atom->label_entity_id = LLKAInternal::duplicateString(label_entity_id);
    atom->label_comp_id = LLKAInternal::duplicateString(label_comp_id);
    atom->label_asym_id = LLKAInternal::duplicateString(label_asym_id);
    atom->auth_atom_id = auth_atom_id ? LLKAInternal::duplicateString(auth_atom_id) : LLKAInternal::duplicateString(label_atom_id);
    atom->auth_comp_id = auth_comp_id ? LLKAInternal::duplicateString(auth_comp_id) : LLKAInternal::duplicateString(label_comp_id);
    atom->auth_asym_id = auth_asym_id ? LLKAInternal::duplicateString(auth_asym_id) : LLKAInternal::duplicateString(label_asym_id);
    atom->coords = *coords;
    atom->label_seq_id = label_seq_id;
    atom->auth_seq_id = auth_seq_id;
    atom->pdbx_PDB_ins_code = LLKAInternal::duplicateString(pdbx_PDB_ins_code),
    atom->pdbx_PDB_model_num = pdbx_PDB_model_num,
    atom->label_alt_id = label_alt_id;
}

LLKA_Structure LLKA_CC LLKA_makeStructure(const LLKA_Atom *atoms, size_t nAtoms)
{
    LLKA_Structure stru{
        nullptr,
        nAtoms
    };

    if (nAtoms < 1)
        return stru;

    stru.atoms  = new LLKA_Atom[stru.nAtoms];
    for (size_t idx = 0; idx < nAtoms; idx++)
        LLKA_duplicateAtom(&atoms[idx], &stru.atoms[idx]);

    return stru;
}


LLKA_Structure LLKA_CC LLKA_makeStructureFromPtrs(const LLKA_Atom *const *atoms, size_t nAtoms)
{
    LLKA_Structure stru{
        nullptr,
        nAtoms
    };

    if (nAtoms < 1)
        return stru;

    stru.atoms = new LLKA_Atom[stru.nAtoms];
    for (size_t idx = 0; idx < nAtoms; idx++)
        LLKA_duplicateAtom(atoms[idx], &stru.atoms[idx]);

    return stru;
}

LLKA_RetCode LLKA_CC LLKA_splitStructureToDinucleotideSteps(const LLKA_Structure *stru, LLKA_Structures *steps)
{
    std::vector<LLKA_Structure> allSteps;

    size_t idx = 0;
    while (idx < stru->nAtoms) {
        const auto &atom = stru->atoms[idx];

        if (!LLKAInternal::isNucleotideCompound(atom.label_comp_id)) {
            idx++;
            continue;
        }

        LLKA_Structure firstNucl = LLKAInternal::extendToResidue({ stru, idx }, atom.pdbx_PDB_model_num, atom.label_asym_id, atom.label_seq_id, LLKA_NO_ALTID, LLKAInternal::EXT_DIR_FWD);
        idx += firstNucl.nAtoms;

        if (idx >= stru->nAtoms) {
            LLKA_destroyStructure(&firstNucl);
            break;
        }

        const auto &atom2 = stru->atoms[idx];
        if (!LLKAInternal::isSameChain(atom, atom2)) {
            LLKA_destroyStructure(&firstNucl);
            continue;
        }

        LLKA_Structure secondNucl = LLKAInternal::extendToResidue({ stru, idx }, atom2.pdbx_PDB_model_num, atom2.label_asym_id, atom2.label_seq_id, LLKA_NO_ALTID, LLKAInternal::EXT_DIR_FWD);

        auto _steps = LLKAInternal::dinucleotideToSteps(firstNucl, secondNucl);
        allSteps.insert(allSteps.end(), _steps.cbegin(), _steps.cend());

        LLKA_destroyStructure(&firstNucl);
        LLKA_destroyStructure(&secondNucl);
    }

    const size_t N = allSteps.size();
    steps->strus = new LLKA_Structure[N];
    steps->nStrus = N;

    std::copy_n(allSteps.cbegin(), N, steps->strus);

    return LLKA_OK;
}
