#include "autoparams.h"
#include "utilities.h"
#include "classes.h"
#include "settings.h"
#include "parameters.h"
#include "optimize.h"
#include "mol2.h"

int main(int argc, char **argv)
{
    // Variable Declarations
    std::stringstream buffer;

    // Initialize program, figure out what is available for use.
    Settings settings(argc, argv);
    // settings.parse_command_line(argc, argv);

    // Load parameters library from known_parameters.dat in the include directory.
    Parameters params;
    params.InitializeParameterLibrary();

    // Initialize secondary variables.
    Frcmod_File frcmod(settings.frcmodfile);

    // read input file (PDB)
    settings.Output("Reading PDB...");
    Molecule mol(settings);
    settings.Output("Checked PDB for unique atom names.");
    settings.Output("Moving molecule to center of mass...");

    // move molecule to center of mass.
    mol.move_to_COM();
    mol.Write_PDB(settings.job_dir, settings.inputfile);
    
    // check total charge/spin combination
    if (mol.SpinChargeValidate())
    {
        settings.Output("Spin/charge combination validated.\n\n");
        buffer.str("");
        buffer << "Spin:   " << mol.spin_mult << "\n";
        buffer << "Charge: " << mol.charge << "\n";
        buffer << "Total Electron Count:" << mol.total_electrons << " \n";
        settings.Output(buffer.str());
    }
    else
    {
        buffer.str("");
        buffer << "Given spin/charge combination is not valid. \n";
        buffer << "Spin:   " << mol.spin_mult << "\n";
        buffer << "Charge: " << mol.charge << "\n";
        buffer << "Total Electron Count:" << mol.total_electrons << " \n";
        settings.Error(buffer.str());
    }

    settings.Output("Analyzing molecular structure...\n\n");
    // identify all bonds
    // by distance?
    mol.FindBonds();
    buffer.str("");
    buffer << "Found " << mol.bonds.size() << " bonds in molecule." << std::endl;
    settings.Output(buffer.str());
    
    // identify all angles
    // by bondpairs with one matching atom
    mol.FindAngles();
    buffer.str("");
    buffer << "Found " << mol.angles.size() << " angles in molecule." << std::endl;
    settings.Output(buffer.str());

    // identify all dihedrals
    // angles with matching middle to other bonds.
    mol.FindTorsions();
    buffer.str("");
    buffer << "Found " << mol.torsions.size() << " torsions in molecule." << std::endl;
    settings.Output(buffer.str());

    // identify all torsions
    // angles with matching ends
    mol.FindDihedrals();
    buffer.str("");
    buffer << "Found " << mol.dihedrals.size() << " dihedrals in molecule." << std::endl;
    settings.Output(buffer.str());
   
    // identify all rings
    mol.FindRings();
    buffer.str("");
    int total_rings = mol.four_rings.size() + mol.five_rings.size() + mol.six_rings.size();
    buffer << "Found " << total_rings << " rings in molecule.  Ring sizes are: " << std::endl;
    buffer << "Four-membered rings: " << mol.four_rings.size() << std::endl;
    buffer << "Five-membered rings: " << mol.five_rings.size() << std::endl;
    buffer << "Six-membered rings:  " << mol.six_rings.size() << std::endl;
    settings.Output(buffer.str());

    // OPTIONAL optimize structure
    if (settings.OPTIMIZE_FIRST) //TODO:  Add TeraChem optimization function.  Quick and dirty, nothing fancy, just get it done.
    {
        // Run TeraChem optimization (flag for GS/ES opt?)
        // confirm successful job completion, if failed, bail out?
        // copy optimized structure to original filename in job_dir.
        settings.Output("Running QM optimization before parametrizing.  This may take some time.");
        TeraChemOpt(settings);
    }
        
    if (!settings.USE_AM1BCC_CHARGES)
    {
        // 2026-09-05: fail loudly, right here, if TeraChem isn't even on
        // PATH -- rather than shelling out to it anyway. silent_shell()
        // never checks exit codes or command-not-found, so the old
        // unconditional shell-out silently produced no resp.out at all;
        // validate_TC_resp_output() then failed to open a file that was
        // never written, parse_TC_resp_output(mol, settings, false) failed
        // the exact same way and returned without setting a single charge,
        // and every atom quietly kept its default-constructed 0.0 --
        // producing a mol2 file full of zero partial charges with no
        // diagnostic anywhere that survives the job directory being cleaned
        // up. This is the incident the user reported: "all of the solvents
        // returned with effectively 0-value partial charges ... those
        // files are not available in any of the job directories". Logging
        // it here means it lands in autoparams.NNNN.err *before* anything
        // is attempted, and the Charges_Are_Degenerate() check below (plus
        // Overseer's own mol2-parsing gate in --finish-autoparams-job) is
        // the safety net that turns this into a real "error" status instead
        // of a silently-cached bad result.
        if (!settings.TC_EXISTS)
        {
            settings.Error("TeraChem not found on PATH -- cannot compute RESP charges. "
                            "Install/license TeraChem and ensure it is on PATH, or rerun "
                            "AutoParams with --am1bcc to request antechamber's AM1-BCC "
                            "charge method instead. Continuing so the rest of this run's "
                            "own diagnostics (bond/angle/dihedral/vdW checks) still execute, "
                            "but the resulting charges will be all-zero and this job will "
                            "be reported as failed below.");
        }
        else
        {
            // calculate RESP charges, apply results to atoms in molecule.
            write_TC_resp_input(settings,mol);
            buffer.str("");
            buffer << "Running TeraChem for RESP charges.  This may take a while..." << std::endl;
            settings.Output(buffer.str());
            std::string curr_path = fs::current_path();
            fs::current_path(settings.job_dir);
            buffer.str("");
            if (DEFAULT_TERACHEM_MODULE != "" && settings.USE_MODULES)
            {
                buffer << "module load "<<DEFAULT_TERACHEM_MODULE<<" && ";
            }
            buffer << "terachem -i resp.in 1> resp.out 2> resp.err";
            silent_shell(buffer.str().c_str());
            buffer.str("");
            fs::current_path(curr_path);

            // validate successful resp job before trying to parse output.
            // If validation fails, there could be an issue with convergence, but the charges may still be useful.
            // We will alert the end user and continue on.
            if (!validate_TC_resp_output(settings))
            {
                settings.Output("RESP calculation did not complete successfully.  Attempting to continue with current charge values.");
                // parse RESP charges
                parse_TC_resp_output(mol, settings, false);
            }
            else
            {
                // parse RESP charges
                parse_TC_resp_output(mol, settings, true);
            }
        }
    }
    else
    {
        // 2026-09-05: --am1bcc currently has nowhere to actually get a
        // charge from -- Generate_Mol2_File()'s antechamber "-c bcc" branch
        // (autoparams.cpp) is dead code, never called from here, and
        // BuildMol2File() below reads straight from mol.atoms[i].resp_charge,
        // which nothing in this branch ever sets. Flagging this explicitly
        // rather than letting it fail silently and get caught only by the
        // generic Charges_Are_Degenerate() check lower down with a less
        // specific message. See README.md's Open design items.
        settings.Error("--am1bcc was requested, but this build has no code path that actually "
                        "assigns AM1-BCC charges to mol.atoms before the mol2 file is written "
                        "(the antechamber \"-c bcc\" plumbing in Generate_Mol2_File() is not "
                        "wired into main()'s control flow). Charges will be all-zero and this "
                        "job will be reported as failed below -- see README.md's Open design "
                        "items for AutoParams.");
    }
    // ************************* Here's where I can put in my molecule atom-typer code...
    AtomTyping(mol);
    
    for (Atom atom : mol.atoms)
    {
        std::cout << atom.atom_name << " : " << atom.atom_type << " : " << atom.resp_charge << std::endl;
    }
    // Generate mol2 file
    BuildMol2File(settings, mol);
    // Generate_Mol2_File(settings);
    // if (!settings.USE_AM1BCC_CHARGES)
    // {
    //     std::cout << "Adding charges to mol2." << std::endl;
    //     Add_Charges_To_Mol2(settings, mol);
    // }

    // 2026-09-05: sanity-check the charges that just got written to disk --
    // see Charges_Are_Degenerate()'s own comment (autoparams.cpp) for the
    // full rationale, including a same-day correction from an aggregate-sum
    // check to a strict per-atom "every single atom near zero" check. Checked
    // here, right after BuildMol2File(), so it reflects exactly what's in
    // settings.mol2file; charges_degenerate gates the final success/failure
    // report and exit code below.
    size_t n_near_zero = 0, n_charge_atoms = 0;
    bool charges_degenerate = Charges_Are_Degenerate(mol, n_near_zero, n_charge_atoms);
    if (charges_degenerate)
    {
        buffer.str("");
        buffer << "AGIMUS_AUTOPARAMS_ZERO_CHARGES atoms_near_zero:" << n_near_zero
               << " total_atoms:" << n_charge_atoms << std::endl;
        std::cout << buffer.str();
        settings.Error("Every one of this molecule's " + std::to_string(n_charge_atoms) +
                       " atoms came back with an effectively-zero partial charge (|charge| < 1e-6). "
                       "See the messages above for the likely cause (TeraChem unavailable, the RESP "
                       "job failing to complete, or --am1bcc having no charge source wired up). " +
                       settings.mol2file + " was written but should NOT be trusted or cached -- "
                       "rerun after fixing the charge source.");
    }

    // check for missing parameters with tleap, parse tleap error file.
    settings.Output("\nIdentifying missing parameters...\n#########################################\n");
    Check_For_Missing_Parameters(settings, mol);

    // OPTIONAL calculate parameters with TeraChem optimizations
        // Each bond should be tested with 0.1A increments to get bond strength
        // Each angle should be tested with 10 degree increments (0-180)
        // Each dihedral should be tested with 10 degree increments (0-360)


    // Generate frcmod file
    settings.Output("Generating frcmod file.");
    Build_Frcmod(frcmod, mol, settings, params);

    // Test mol2/frcmod file combination in tleap
    Check_For_Missing_Parameters(settings, mol);
    std::string prmtop = settings.job_dir + "/tmp.prmtop";
    std::string restart = settings.job_dir + "/tmp.rst7";

    if (!CheckFileExists(restart) || !CheckFileExists(prmtop))
    {
        settings.Output("Attempting to regenerate frcmod file for new missing parameters.");
        Build_Frcmod(frcmod, mol, settings, params);
        // Test mol2/frcmod file combination in tleap
        Check_For_Missing_Parameters(settings, mol);
    }

    // if successful, return results.
    bool prmtop_ok = CheckFileExists(restart) && CheckFileExists(prmtop);
    if (prmtop_ok)
    {
        settings.Output("Successfully generated prmtop/rst7 with new parameters.");
        buffer.str("");
        buffer << "Mol2 File:   " << settings.mol2file << std::endl;
        buffer << "Frcmod File: " << settings.frcmodfile << std::endl;
        settings.Output(buffer.str());
    }
    else
    {
        settings.Error("Unable to generate prmtop/rst7 files, check leap.log output for possible issues.");
    }

    // 2026-09-05: previously this always `return 0`, even on the branch
    // above that just logged a failure -- so nothing calling this binary
    // (a human, or Overseer's --finish-autoparams-job) could tell success
    // from failure via exit code alone. Now reflects both known failure
    // modes: a bad prmtop/rst7 test build, and the zero-charge check above.
    // (Overseer's own --finish-autoparams-job re-checks the mol2/frcmod
    // files and their charges independently rather than trusting this exit
    // code, since it runs in a separate shell-chained process -- but a
    // correct exit code here still matters for anyone running `autoparams`
    // directly, e.g. via install_external_tools.sh's verification or by
    // hand.)
    if (charges_degenerate)
    {
        settings.Error("Refusing to report success: partial charges are degenerate (see above).");
    }
    return (prmtop_ok && !charges_degenerate) ? 0 : 1;
}
