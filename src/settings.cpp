#include "settings.h"
#include "classes.h"   // is_atom_record(), is_known_element()

#include <cerrno>
#include <climits>
#include <cstdlib>

void PrintUsage(std::ostream &os)
{
    os <<
"AutoParams - generate AMBER parameters (mol2 + frcmod) for a small molecule,\n"
"ligand, solvent, or nonstandard residue.\n"
"\n"
"Usage:\n"
"  autoparams -i <file.pdb> [options]\n"
"  autoparams -h | --help\n"
"\n"
"AutoParams reads one PDB structure, assigns atom types, computes RESP partial\n"
"charges with TeraChem, fills in any bond/angle/dihedral/vdW parameters AMBER\n"
"lacks from its own library (known_parameters.dat), and test-builds the result\n"
"in tleap.\n"
"\n"
"Required:\n"
"  -i, --input <file.pdb>     Input structure. Must be a file in the current\n"
"                             working directory (a bare name or ./name).\n"
"                             NOTE: the file is rewritten in place (atoms\n"
"                             renumbered, HETATM written as ATOM, non-atom\n"
"                             records dropped) -- keep a copy if you need one.\n"
"\n"
"Molecule:\n"
"  -c, --charge <int>         Net formal charge. Default: 0.\n"
"  -s, --spin <int>           Spin multiplicity, >= 1. Default: 1.\n"
"  -f, --forcefield <name>    Base force field for the tleap test build\n"
"                             (case-insensitive). Default: PROTEIN.\n"
"                               PROTEIN       source leaprc.protein.ff14SB\n"
"                               DNA           source leaprc.DNA.OL15\n"
"                               RNA           source leaprc.RNA.OL3\n"
"                               CARBOHYDRATE  source leaprc.GLYCAM_06j-1\n"
"                               NONE          no base leaprc: the frcmod\n"
"                                             supplies every parameter the\n"
"                                             tleap test reports missing\n"
"                                             (Overseer's default).\n"
"                                             GAFF2 is an alias for NONE.\n"
"                             Atom types are the same parm10-style types\n"
"                             (CT, OH, HC, ...) whichever is chosen.\n"
"      --head <atom>          Atom bonded to the preceding residue (polymer\n"
"                             head). Default: none.\n"
"  -t, --tail <atom>          Atom bonded to the following residue (polymer\n"
"                             tail). Default: none.\n"
"  -d, --dummy <atom>         Capping/dummy atom left out of the RESP fit.\n"
"                             Repeat the flag for several atoms.\n"
"                             Atoms can also be tagged in the PDB itself: an\n"
"                             ATOM/HETATM line containing HEAD, TAIL, or DUMMY\n"
"                             after its coordinates marks that atom. PDB tags\n"
"                             take precedence over --head/--tail.\n"
"\n"
"Charges and QM:\n"
"  -o, --optimize             TeraChem gas-phase geometry optimization before\n"
"                             the RESP fit. Skipped (and logged as an error)\n"
"                             if TeraChem is not available.\n"
"      --tckw <key> <value>   Override one TeraChem keyword in resp.in/opt.in.\n"
"      --tckw <key>=<value>   Repeatable, e.g. --tckw basis 6-31g or\n"
"                             --tckw method=wb97x. The run type cannot be\n"
"                             changed. Defaults: method b3lyp, basis 6-31gss,\n"
"                             convthre 1e-7, threall 1e-14, precision mixed,\n"
"                             maxit 200, scf diis+a, gpus 1, gpumem 256.\n"
"      --am1bcc               Request AM1-BCC charges instead of RESP.\n"
"                             KNOWN GAP: no AM1-BCC charge source is wired in\n"
"                             yet; such a run ends with all-zero charges and\n"
"                             exit status 1.\n"
"\n"
"Environment:\n"
"      --container            Running inside the AGIMUS container: never try\n"
"                             `module load` for TeraChem.\n"
"  -h, --help                 Print this help and exit.\n"
"\n"
"Output (current directory; NNNN = first unused number, starting at 0000):\n"
"  autoparams.NNNN.out        Progress log.\n"
"  autoparams.NNNN.err        Error log (created only if an error is logged).\n"
"  autoparams.NNNN/           Job directory: <name>.mol2 and <name>.frcmod\n"
"                             (the parameters), plus resp.in/resp.out,\n"
"                             tleap.in, leap.log, tmp.prmtop/tmp.rst7.\n"
"\n"
"External programs: terachem (RESP charges, --optimize) and tleap (test build)\n"
"on PATH. Outside --container mode, `module load` of the TeraChem module\n"
"configured in include/config.h is tried when terachem is not on PATH.\n"
"known_parameters.dat is read from <install>/include/, next to\n"
"<install>/bin/autoparams (symlinks to the binary are resolved).\n"
"\n"
"Exit status: 0 on success; 1 on a usage error, a missing input or parameter\n"
"file, a failed tleap test build, or all-zero partial charges.\n"
"\n"
"Under AGIMUS:\n"
"  overseer --add-task --series-id <id> --module AutoParams \\\n"
"           --param input=<file.pdb> [--param charge=<n>] [--param spin=<n>] \\\n"
"           [--param forcefield=<name>] [--param library-dir=<dir>] ...\n"
"  Each --param key=value becomes --key value (short keys i, c, s, f, o, h, t,\n"
"  d are mapped to their long names). Overseer copies the input into the\n"
"  task's job directory and runs autoparams there, adds --forcefield NONE\n"
"  unless one is given, adds --charge from a completed FeatureFinder task on\n"
"  the same input unless a charge is given, adds --container in container\n"
"  mode, then runs `overseer --finish-autoparams-job`, which checks the mol2/\n"
"  frcmod output, registers it under <library-dir>/parameters/, and reports\n"
"  the task ok or error.\n";
}

namespace
{
    [[noreturn]] void usage_error(const std::string &msg)
    {
        std::cerr << "ERROR: " << msg << "\n"
                  << "Run 'autoparams --help' for usage." << std::endl;
        std::exit(1);
    }

    bool parse_int(const std::string &s, int &out)
    {
        if (s.empty()) return false;
        errno = 0;
        char *end = nullptr;
        long v = std::strtol(s.c_str(), &end, 10);
        if (errno != 0 || end == s.c_str() || *end != '\0') return false;
        if (v < INT_MIN || v > INT_MAX) return false;
        out = static_cast<int>(v);
        return true;
    }

    [[noreturn]] void input_error(const std::string &msg)
    {
        std::cerr << "ERROR: " << msg << std::endl;
        std::exit(1);
    }

    // Read-only check of the input PDB before anything is written: at least
    // one ATOM/HETATM record, and every one of them carrying a real element
    // symbol in columns 77-78 -- the only place AutoParams takes the element
    // from (mass, electron count for the spin check, vdW radius for bond
    // perception all come from it). A missing element column used to crash
    // the run (substr past the end of the line); an unknown one silently
    // counted as zero electrons and zero mass.
    void validate_input_pdb(const std::string &path)
    {
        std::ifstream in(path);
        if (!in.is_open()) input_error("cannot read input file: " + path);
        std::string line;
        size_t lineno = 0, n_atoms = 0;
        while (std::getline(in, line))
        {
            lineno++;
            if (!is_atom_record(line)) continue;
            n_atoms++;
            std::string padded = line;
            if (padded.size() < 80) padded.resize(80, ' ');
            std::string element = trim_whitespace(padded.substr(76, 2), " \t\r");
            if (element.empty())
                input_error(path + ", line " + std::to_string(lineno) +
                            ": atom record has no element symbol in columns 77-78 "
                            "(AutoParams reads the element only from there).");
            if (!is_known_element(element))
                input_error(path + ", line " + std::to_string(lineno) +
                            ": unknown element symbol '" + element + "' in columns 77-78.");
        }
        if (n_atoms == 0) input_error(path + " contains no ATOM or HETATM records.");
    }

    // A flag's value may not itself look like an option -- that almost
    // always means the value was left off ("autoparams -i -c 1"). Negative
    // integers ("-c -1") are values, not options.
    bool looks_like_option(const std::string &s)
    {
        int dummy;
        return s.size() > 1 && s[0] == '-' && !parse_int(s, dummy);
    }
}

void Settings::Output(std::string output)
{
    std::ofstream outfile_stream;
    if (!CheckFileExists(outfile))
    {
        outfile_stream.open(outfile,std::ios::out);
        outfile_stream << FormattedTimeStamp();
        outfile_stream << "Initializing AutoParams Output..." << std::endl;
    }
    else
    {
        outfile_stream.open(outfile,std::ios::app);
    }
    outfile_stream << output;
    if (output.empty() || output.back() != '\n')
    {
        outfile_stream << "\n";
    }
    outfile_stream.close();
}

void Settings::Error(std::string error)
{
    std::ofstream errfile_stream;
    if (!CheckFileExists(errfile))
    {
        errfile_stream.open(errfile,std::ios::out);
        errfile_stream << FormattedTimeStamp();
        errfile_stream << "Initialize AutoParams Error Logging." << std::endl;
    }
    else
    {
        errfile_stream.open(errfile,std::ios::app);
    }
    errfile_stream << error;
    if (error.empty() || error.back() != '\n')
    {
        errfile_stream << "\n";
    }
    errfile_stream.close();
}

void Settings::CheckPrograms()
{
    Output("Locating External Programs...\n");
    TC_EXISTS = CheckProgramExists("terachem");

    if ((!TC_EXISTS) && (USE_MODULES) && (DEFAULT_TERACHEM_MODULE != ""))
    {
        TC_EXISTS = CheckProgramExists("terachem", DEFAULT_TERACHEM_MODULE);
    }

    PSI4_EXISTS = CheckProgramExists("psi4");
    AIMNET_EXISTS = CheckProgramExists("aimnet2");
    TLEAP_EXISTS = CheckProgramExists("tleap");
    PARMED_EXISTS = CheckProgramExists("parmed");
    CPPTRAJ_EXISTS = CheckProgramExists("cpptraj");
    ANTECHAMBER_EXISTS = CheckProgramExists("antechamber");

    if (TC_EXISTS) { Output("TeraChem located."); }
    if (PSI4_EXISTS) { Output("Psi4 located."); }
    if (AIMNET_EXISTS) { Output("AimNET2 located."); }
    if (TLEAP_EXISTS) { Output("Tleap located."); }
    if (PARMED_EXISTS) { Output("Parmed located."); }
    if (CPPTRAJ_EXISTS) { Output("CPPTRAJ located."); }
    if (ANTECHAMBER_EXISTS) { Output("Antechamber located."); }
    Output("\n");
}

void Settings::SetUpLogFiles()
{
    // Set up output and error files, ensuring numerical naming and matched
    // pairs with the job folder: the first NNNN for which none of
    // autoparams.NNNN.out, autoparams.NNNN.err, autoparams.NNNN/ exists.
    // All three are stored as absolute paths -- several later steps chdir
    // into job_dir (tleap, TeraChem) and still call Output()/Error(), and a
    // relative log path would then silently start a second log inside the
    // job directory.
    for (int i = 0; ; i++)
    {
        std::stringstream stem;
        stem << "autoparams." << std::setw(4) << std::setfill('0') << i;
        std::string out = stem.str() + ".out";
        std::string err = stem.str() + ".err";
        std::string dir = stem.str() + "/";
        if (!CheckFileExists(out) && !CheckFileExists(err) && !fs::exists(dir))
        {
            outfile = fs::absolute(out).string();
            errfile = fs::absolute(err).string();
            job_dir = fs::absolute(dir).string();
            break;
        }
    }
    // Create job folder (scratch, etc.)
    if (!fs::exists(job_dir))
    {
        fs::create_directory(job_dir);
    }
}

// Pure argument parsing and validation: no files, directories, or shell
// commands are touched here, so `--help` and every usage error leave the
// current directory exactly as it was. Exits 0 after printing --help, and
// exits 1 (message on stderr) on any usage error.
void Settings::parse_command_line(int argc,char **argv)
{
    mol_charge = 0;
    mol_spin = 1;
    forcefield = "PROTEIN";
    inputfile = "";
    OPTIMIZE_REQUESTED = false;
    OPTIMIZE_FIRST = false;
    INCLUDE_TIMESTAMPS = false;
    USE_AM1BCC_CHARGES = false;
    head_atom_name = "0";
    tail_atom_name = "0";
    dummy_atom_names = {};
    tc_keys = {};
    CONTAINER_MODE = false;
    USE_MODULES = false;
    TC_EXISTS = PSI4_EXISTS = AIMNET_EXISTS = TLEAP_EXISTS = false;
    PARMED_EXISTS = CPPTRAJ_EXISTS = ANTECHAMBER_EXISTS = false;

    std::vector<std::string> args(argv + 1, argv + argc);

    // --help/-h wins wherever it appears, before anything else is judged.
    for (const std::string &a : args)
    {
        if (a == "-h" || a == "--help")
        {
            PrintUsage(std::cout);
            std::exit(0);
        }
    }

    auto value_of = [&](size_t &i, const std::string &flag) -> std::string
    {
        if (i + 1 >= args.size() || looks_like_option(args[i + 1]))
        {
            usage_error(flag + " requires a value.");
        }
        return args[++i];
    };

    for (size_t i = 0; i < args.size(); i++)
    {
        const std::string &a = args[i];
        if (a == "-i" || a == "--input")
        {
            inputfile = value_of(i, a);
        }
        else if (a == "-c" || a == "--charge")
        {
            std::string v = value_of(i, a);
            if (!parse_int(v, mol_charge)) usage_error(a + " must be an integer (got '" + v + "').");
        }
        else if (a == "-s" || a == "--spin")
        {
            std::string v = value_of(i, a);
            if (!parse_int(v, mol_spin)) usage_error(a + " must be an integer (got '" + v + "').");
            if (mol_spin < 1) usage_error(a + ": spin multiplicity must be >= 1 (got " + v + ").");
        }
        else if (a == "-f" || a == "--forcefield")
        {
            std::string given = value_of(i, a);
            forcefield = given;
            transform(forcefield.begin(), forcefield.end(), forcefield.begin(),::toupper);
            // GAFF2 is what Overseer passed before 2026-09-28. It was never a
            // name this tool knew -- it simply matched no leaprc, which is
            // exactly NONE's behaviour -- so it is kept as an alias.
            if (forcefield == "GAFF2") forcefield = "NONE";
            static const std::set<std::string> KNOWN = {"PROTEIN", "DNA", "RNA", "CARBOHYDRATE", "NONE"};
            if (KNOWN.find(forcefield) == KNOWN.end())
            {
                usage_error(a + ": unknown force field '" + given +
                            "'. Use PROTEIN, DNA, RNA, CARBOHYDRATE, or NONE (GAFF2 is accepted as NONE).");
            }
        }
        else if (a == "-o" || a == "--optimize")
        {
            OPTIMIZE_REQUESTED = true;
        }
        else if (a == "--head")
        {
            head_atom_name = value_of(i, a);
        }
        else if (a == "-t" || a == "--tail")
        {
            tail_atom_name = value_of(i, a);
        }
        else if (a == "-d" || a == "--dummy")
        {
            dummy_atom_names.push_back(value_of(i, a));
        }
        else if (a == "--am1bcc")
        {
            USE_AM1BCC_CHARGES = true;
        }
        else if (a == "--tckw")
        {
            // Two accepted forms: "--tckw KEY VALUE" (original) and
            // "--tckw KEY=VALUE" (a single token -- the only form an Overseer
            // --param can produce, since each --param becomes one flag and
            // one value).
            std::string kv = value_of(i, a);
            size_t eq = kv.find('=');
            std::string key, val;
            if (eq != std::string::npos)
            {
                key = kv.substr(0, eq);
                val = kv.substr(eq + 1);
            }
            else
            {
                key = kv;
                if (i + 1 >= args.size() || looks_like_option(args[i + 1]))
                    usage_error("--tckw requires a keyword and a value: --tckw KEY VALUE or --tckw KEY=VALUE.");
                val = args[++i];
            }
            if (key.empty() || val.empty())
                usage_error("--tckw requires a keyword and a value: --tckw KEY VALUE or --tckw KEY=VALUE (got '" + kv + "').");
            tc_keys[key] = val;
        }
        else if (a == "--container")
        {
            CONTAINER_MODE = true;
        }
        else
        {
            std::string hint;
            // "--i", "--c", ... : a short flag written with two dashes (the
            // shape an Overseer "--param c=1" used to produce).
            if (a.size() == 3 && a[0] == '-' && a[1] == '-' && std::isalpha(static_cast<unsigned char>(a[2])))
                hint = " (short options take one dash, e.g. -" + a.substr(2) + ")";
            usage_error("Unknown option '" + a + "'" + hint + ".");
        }
    }

    if (inputfile.empty())
    {
        usage_error("an input structure is required: -i/--input <file.pdb>.");
    }
    if (!fs::exists(inputfile))
    {
        usage_error("input file not found: " + inputfile);
    }
    if (!fs::is_regular_file(inputfile))
    {
        usage_error("input is not a regular file: " + inputfile);
    }
    // The rest of the program uses the input name both to read the original
    // (from the current directory) and as a file name inside the job
    // directory (autoparams.NNNN/<name>, <stem>.mol2, <stem>.frcmod,
    // TeraChem's coordinates keyword, tleap's loadpdb). A name with a
    // directory component breaks the second use, so accept only files that
    // live in the current directory and reduce them to their bare name.
    fs::path in(inputfile);
    fs::path parent = in.parent_path();
    if (!parent.empty() && parent != ".")
    {
        std::error_code ec;
        if (!fs::equivalent(parent, fs::current_path(), ec) || ec)
        {
            usage_error("the input file must be in the current working directory (got '" + inputfile +
                        "'). cd into its directory, or copy it here, and pass the bare file name.");
        }
    }
    inputfile = in.filename().string();
    validate_input_pdb(inputfile);

    // Only probe for Lmod/`module` outside container mode (it writes and
    // removes scratch files, so it is deliberately not done before
    // validation has passed).
    USE_MODULES = CONTAINER_MODE ? false : CheckProgramExists("module");
}

void Settings::LogSettings()
{
    Output("Parsing command line arguments...\n");
    std::stringstream buffer;
    buffer.str("");
    buffer << "Input coordinates: " << inputfile << std::endl;
    buffer << "Molecular charge:  " << mol_charge << std::endl;
    buffer << "Molecular spin:    " << mol_spin << std::endl;
    buffer << "Base Forcefield:   " << forcefield << std::endl;
    buffer << "Mol2 filename:     " << string_split(mol2file,'/').back() << std::endl;
    buffer << "Frcmod filename:   " << string_split(frcmodfile,'/').back() << std::endl;
    for (const auto &kv : tc_keys)
    {
        buffer << "TeraChem keyword:  " << kv.first << " " << kv.second << std::endl;
    }
    buffer << "\n";
    Output(buffer.str());
}

Settings::Settings(int argc, char **argv)
{
    parse_command_line(argc, argv);   // exits on --help or any usage error
    SetUpLogFiles();

    std::string stem = inputfile.substr(0, inputfile.find_last_of('.'));
    mol2file = stem + ".mol2";
    frcmodfile = job_dir + stem + ".frcmod";

    LogSettings();
    CheckPrograms();

    // -o/--optimize can only be honoured once CheckPrograms() has looked for
    // TeraChem (it used to be judged during argument parsing, before
    // TC_EXISTS had been set at all).
    if (OPTIMIZE_REQUESTED)
    {
        if (TC_EXISTS)
        {
            Output("Gas phase QM optimization will be performed before RESP fitting.\n\tThis may take a while.");
            OPTIMIZE_FIRST = true;
        }
        else
        {
            Error("Optimization requested but TeraChem is not available.  Skipping optimization.");
            OPTIMIZE_FIRST = false;
        }
    }
    QuickParsePDB();
}

Settings::~Settings()
{

}

void Settings::QuickParsePDB()
{
    // HEAD/TAIL/DUMMY tags are only meaningful on atom records (the atom
    // name is read from columns 13-16); other records -- notably HEADER,
    // which contains "HEAD" -- are ignored.
    std::ifstream ifile(inputfile,std::ios::in);
    std::string line;
    while (getline(ifile,line))
    {
        if (!is_atom_record(line)) continue;
        if (line.size() < 16) continue;
        if (line.find("DUMMY")!=std::string::npos)
        {
            dummy_atom_names.push_back(trim_whitespace(line.substr(12,4)));
        }
        if (line.find("HEAD")!= std::string::npos)
        {
            head_atom_name = trim_whitespace(line.substr(12,4));
        }
        if (line.find("TAIL")!=std::string::npos)
        {
            tail_atom_name = trim_whitespace(line.substr(12,4));
        }
    }
    ifile.close();
}
