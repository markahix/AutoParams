#ifndef AUTOPARAMS_H
#define AUTOPARAMS_H

#include "utilities.h"
#include "classes.h"
#include "settings.h"
#include "parameters.h"

void write_TC_resp_input(Settings settings, Molecule mol);
bool validate_TC_resp_output(Settings settings);
void parse_TC_resp_output(Molecule &mol,Settings settings, bool resp_completed);
void BuildMol2File(Settings settings, Molecule mol);
void Generate_Mol2_File(Settings settings);
void Validate_Mol2_File(Settings settings);
void Add_Charges_To_Mol2(Settings settings, Molecule &mol);
void Check_For_Missing_Parameters(Settings settings, Molecule &mol);
void Build_Frcmod(Frcmod_File &frcmod, Molecule mol, Settings settings, Parameters params);

// 2026-09-05 (corrected same day per user review -- see autoparams.cpp for
// the full before/after rationale): sanity-checks a just-built Molecule's
// per-atom partial charges. Reports degenerate only if EVERY atom's charge
// is near zero (|charge| < 1e-6) -- never based on an aggregate/net sum, so
// a genuinely neutral molecule with real per-atom charges is never flagged,
// and a partially-broken result (some atoms real, most still degenerate)
// is never masked by the real atoms' contribution to a sum.
bool Charges_Are_Degenerate(const Molecule &mol, size_t &n_near_zero, size_t &n_atoms);

#endif