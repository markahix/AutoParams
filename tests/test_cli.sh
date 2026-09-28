#!/bin/bash
# test_cli.sh -- behavioural checks for AutoParams' command-line handling
# (--help, argument validation, exit codes, parameter-library lookup).
#
# Usage: tests/test_cli.sh [-h|--help] [<AutoParams-dir>]
#   <AutoParams-dir>  directory holding bin/autoparams and
#                     include/known_parameters.dat (default: this script's
#                     parent directory, i.e. the AutoParams tree it ships in).
#   Build first (make). Every case runs in a fresh scratch directory; the
#   script prints PASS/FAIL per case and exits 1 if any case failed. Needs
#   no TeraChem or AmberTools (runs that get past argument handling are only
#   checked up to their first log lines).
HERE=$(cd "$(dirname "$0")" && pwd)
if [[ "$1" == "--help" || "$1" == "-h" || $# -gt 1 ]]; then
    sed -n '2,12p' "$0"; exit 0
fi
MOD=$(cd "${1:-$HERE/..}" && pwd)
BIN="$MOD/bin/autoparams"
[[ -x "$BIN" ]] || { echo "ERROR: $BIN not found or not executable (run make first)" >&2; exit 1; }
PDB_SRC="$HERE/methanol.pdb"
FAILS=0; N=0
SCR=$(mktemp -d)

pass() { N=$((N+1)); echo "PASS  $1"; }
fail() { N=$((N+1)); FAILS=$((FAILS+1)); echo "FAIL  $1"; [[ -n "$2" ]] && echo "      $2"; }

# run_case <name> <expected-exit> <stdout-regex|-> <stderr-regex|-> <expect-no-files:0/1> -- args...
run_case() {
    local name=$1 want=$2 out_re=$3 err_re=$4 nofiles=$5; shift 6
    local d; d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/methanol.pdb"
    ( cd "$d" && timeout 60 "$@" >"$d/.stdout" 2>"$d/.stderr" ); local rc=$?
    local extra; extra=$(cd "$d" && ls -A | grep -v -e '^\.stdout$' -e '^\.stderr$' -e '^methanol\.pdb$' | tr '\n' ' ')
    local why=""
    [[ $rc -ne $want ]] && why+="exit=$rc (want $want) "
    [[ "$out_re" != "-" ]] && ! grep -Eq -- "$out_re" "$d/.stdout" && why+="stdout lacks /$out_re/ "
    [[ "$err_re" != "-" ]] && ! grep -Eq -- "$err_re" "$d/.stderr" && why+="stderr lacks /$err_re/ "
    [[ $nofiles -eq 1 && -n "$extra" ]] && why+="created files: $extra"
    if [[ -z "$why" ]]; then pass "$name"; else
        fail "$name" "$why"; echo "      stderr: $(head -c 300 "$d/.stderr" | tr '\n' '|')"; fi
}

A=( "$BIN" )
run_case "--help prints usage, exit 0, no files"   0 'Usage:' - 1 -- "${A[@]}" --help
run_case "-h prints usage, exit 0, no files"       0 'Usage:' - 1 -- "${A[@]}" -h
run_case "--help wins over other args"            0 'Usage:' - 1 -- "${A[@]}" -i nonexist.pdb --bogus --help
for f in --input --charge --spin --forcefield --optimize --head --tail --dummy --am1bcc --tckw --container --help; do
    run_case "--help documents $f"                0 "$f" - 1 -- "${A[@]}" --help
done
run_case "no args -> exit 1, names --input"        1 - '--input' 1 -- "${A[@]}"
run_case "missing input value"                     1 - 'requires a value' 1 -- "${A[@]}" --input
run_case "missing charge value (next is flag)"     1 - 'requires a value' 1 -- "${A[@]}" -i methanol.pdb -c --spin 1
run_case "nonexistent input"                       1 - 'not found' 1 -- "${A[@]}" -i nonexist.pdb
run_case "unknown long option"                     1 - "Unknown option '--bogus'" 1 -- "${A[@]}" -i methanol.pdb --bogus
run_case "Overseer-style --i is rejected w/ hint"  1 - "Unknown option '--i'.*-i" 1 -- "${A[@]}" --i methanol.pdb
run_case "stray positional rejected"               1 - "Unknown option 'extra'" 1 -- "${A[@]}" -i methanol.pdb extra
run_case "non-integer charge"                      1 - 'integer' 1 -- "${A[@]}" -i methanol.pdb -c abc
run_case "spin 0 rejected"                         1 - 'spin' 1 -- "${A[@]}" -i methanol.pdb -s 0
run_case "--tckw needs KEY VALUE"                  1 - 'tckw' 1 -- "${A[@]}" -i methanol.pdb --tckw method
run_case "input in other dir rejected clearly"     1 - 'current working directory' 1 -- "${A[@]}" -i "$HERE/methanol.pdb"

# Negative charge value must be accepted as a value, both --tckw forms must
# parse, and a valid run must get past argument handling into the real work
# (log file + job dir appear, settings summary logged). Exit status is not
# checked here: without TeraChem/AmberTools the run itself fails later.
d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"
( cd "$d" && timeout 60 "$BIN" -i ./methanol.pdb -c -1 -s 2 --tckw method b3lyp --tckw basis=6-31g >"$d/.o" 2>"$d/.e" )
if [[ -f "$d/autoparams.0000.out" && -d "$d/autoparams.0000" ]] \
   && grep -q 'Molecular charge:  -1' "$d/autoparams.0000.out" \
   && grep -q 'Molecular spin:    2' "$d/autoparams.0000.out" \
   && ! grep -q 'Unknown option\|requires a value' "$d/.e"; then pass "valid args (-c -1, ./input, both --tckw forms) start a run"
else fail "valid args (-c -1, ./input, both --tckw forms) start a run" "$(head -c 300 "$d/.e" | tr '\n' '|')"; fi
# A second run in the same directory must keep its log absolute (all lines in
# autoparams.0001.out, none leaked into autoparams.0001/ by the chdir).
( cd "$d" && timeout 60 "$BIN" -i methanol.pdb >/dev/null 2>&1 )
if grep -q 'Validating non-connecting molecule' "$d/autoparams.0001.out" 2>/dev/null \
   && ! ls "$d/autoparams.0001/autoparams."* >/dev/null 2>&1; then pass "second run logs stay in autoparams.0001.out"
else fail "second run logs stay in autoparams.0001.out" "$(ls "$d" "$d/autoparams.0001" 2>&1 | tr '\n' ' ')"; fi

# Parameter-file lookup: via a symlink in an unrelated dir, not on PATH.
L=$(mktemp -d -p "$SCR"); ln -s "$BIN" "$L/autoparams"
d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"
( cd "$d" && env PATH=/usr/bin:/bin timeout 60 "$L/autoparams" -i methanol.pdb >"$d/.o" 2>"$d/.e" )
if grep -q "Loading known parameters from \"*$MOD/include/known_parameters.dat" "$d/.o"; then pass "symlinked binary off PATH finds parameter file"
else fail "symlinked binary off PATH finds parameter file" "$(head -2 "$d/.o" | tr '\n' '|') $(head -c 200 "$d/.e")"; fi

# Parameter file genuinely missing -> exit 1 with an error on stderr.
C=$(mktemp -d -p "$SCR"); mkdir -p "$C/bin"; cp "$BIN" "$C/bin/"
d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"
( cd "$d" && env PATH=/usr/bin:/bin timeout 60 "$C/bin/autoparams" -i methanol.pdb >"$d/.o" 2>"$d/.e" ); rc=$?
if [[ $rc -eq 1 ]] && grep -q 'known_parameters.dat' "$d/.e"; then pass "missing parameter file -> exit 1 + stderr"
else fail "missing parameter file -> exit 1 + stderr" "exit=$rc stderr=$(head -c 200 "$d/.e")"; fi

# --- force field names -------------------------------------------------------
run_case "unknown force field rejected"            1 - "unknown force field 'GLYCAM'" 1 -- "${A[@]}" -i methanol.pdb -f GLYCAM
for ff in NONE gaff2; do
    d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"
    ( cd "$d" && timeout 60 "$BIN" -i methanol.pdb -f $ff >/dev/null 2>&1 )
    if grep -q 'Base Forcefield:   NONE' "$d/autoparams.0000.out" \
       && ! grep -q '^source leaprc\.\(protein\|DNA\|RNA\|GLYCAM\)' "$d/autoparams.0000/tleap.in" \
       && grep -q 'built without a base force field' "$d/autoparams.0000/methanol.frcmod"; then pass "-f $ff runs as NONE (no base leaprc)"
    else fail "-f $ff runs as NONE (no base leaprc)" "$(head -3 "$d/autoparams.0000/tleap.in" 2>&1 | tr '\n' '|')"; fi
done

# --- input PDB content ---------------------------------------------------------
mkpdb() { # mkpdb <out> <python expression editing list L of lines>
    python3 - "$PDB_SRC" "$1" "$2" <<'EOF'
import sys
L = open(sys.argv[1]).read().splitlines()
exec(sys.argv[3])
open(sys.argv[2], 'w').write('\n'.join(L) + '\n')
EOF
}
atom_idx='i=next(k for k,l in enumerate(L) if l.startswith(("ATOM","HETATM")))'
check_input_error() { # <name> <pdb-edit> <stderr-regex>
    local d; d=$(mktemp -d -p "$SCR")
    mkpdb "$d/bad.pdb" "$2"
    ( cd "$d" && timeout 60 "$BIN" -i bad.pdb >"$d/.o" 2>"$d/.e" ); local rc=$?
    local extra; extra=$(cd "$d" && ls -A | grep -v -e '^\.o$' -e '^\.e$' -e '^bad\.pdb$' | tr '\n' ' ')
    if [[ $rc -eq 1 && -z "$extra" ]] && grep -Eq -- "$3" "$d/.e"; then pass "$1"
    else fail "$1" "exit=$rc files=[$extra] stderr=$(head -c 200 "$d/.e")"; fi
}
check_input_error "atom line without element column -> exit 1" "$atom_idx; L[i]=L[i][:66]" 'no element symbol in columns 77-78'
check_input_error "unknown element symbol -> exit 1"            "$atom_idx; l=L[i].ljust(80); L[i]=l[:76]+'Xq'+l[78:]" "unknown element symbol 'Xq'"
check_input_error "no atom records -> exit 1"                   "L=[l for l in L if not l.startswith(('ATOM','HETATM'))]" 'no ATOM or HETATM records'

d=$(mktemp -d -p "$SCR"); mkpdb "$d/rem.pdb" "L.insert(0,'REMARK   this line mentions ATOM but is not an atom')"
( cd "$d" && timeout 60 "$BIN" -i rem.pdb >"$d/.o" 2>&1 )
n=$(grep -c '^Has element' "$d/.o")
if [[ $n -eq 6 ]]; then pass "REMARK line mentioning ATOM is not read as an atom"
else fail "REMARK line mentioning ATOM is not read as an atom" "atoms read: $n (want 6)"; fi

# --- charges and scratch files -------------------------------------------------
d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"; echo keep >"$d/out"; echo keep >"$d/err"
( cd "$d" && timeout 60 "$BIN" -i methanol.pdb >"$d/.o" 2>&1 )
nz=$(grep -Ec '^[A-Z0-9]+ : [A-Za-z0-9]+ : 0$' "$d/.o")
if [[ $nz -eq 6 ]]; then pass "charges without TeraChem are exactly 0 (not uninitialized memory)"
else fail "charges without TeraChem are exactly 0 (not uninitialized memory)" "$(grep ' : ' "$d/.o" | head -3 | tr '\n' '|')"; fi
if [[ "$(cat "$d/out" "$d/err" 2>/dev/null)" == $'keep\nkeep' ]]; then pass "user files named out/err are left alone"
else fail "user files named out/err are left alone" "out/err missing or changed"; fi

# --- -o/--optimize: the optimized geometry is the one used ----------------------
# A stand-in terachem: for opt.in it writes scr/optim.pdb (last MODEL = input
# coordinates scaled by 1.1) and "Job finished:"; for resp.in it does nothing.
TC=$(mktemp -d -p "$SCR"); cat >"$TC/terachem" <<'EOF'
#!/bin/bash
while [[ $# -gt 0 ]]; do [[ "$1" == "-i" ]] && IN=$2; shift; done
[[ "$IN" == "opt.in" ]] || exit 0
c=$(awk '$1=="coordinates"{print $2}' opt.in); mkdir -p scr
{ echo "MODEL        1"; grep -E '^(ATOM|HETATM)' "$c"; echo "ENDMDL"; echo "MODEL        2"
  grep -E '^(ATOM|HETATM)' "$c" | awk '{printf "%s%8.3f%8.3f%8.3f%s\n", substr($0,1,30), substr($0,31,8)*1.1, substr($0,39,8)*1.1, substr($0,47,8)*1.1, substr($0,55)}'
  echo "ENDMDL"; } > scr/optim.pdb
echo "Job finished: stand-in"
EOF
chmod +x "$TC/terachem"
d=$(mktemp -d -p "$SCR"); cp "$PDB_SRC" "$d/"; cp "$PDB_SRC" "$d/orig.pdb"
( cd "$d" && PATH="$TC:$PATH" timeout 60 "$BIN" -i methanol.pdb -o >/dev/null 2>&1 )
ratio=$(python3 - "$d/orig.pdb" "$d/autoparams.0000/methanol.pdb" <<'EOF'
import sys, math
xyz = lambda f: [tuple(float(l[30+8*k:38+8*k]) for k in range(3)) for l in open(f) if l.startswith(('ATOM','HETATM'))]
a, b = xyz(sys.argv[1]), xyz(sys.argv[2])
print('%.2f' % (math.dist(b[0], b[1]) / math.dist(a[0], a[1])) if len(a) == len(b) and a else 'n/a')
EOF
)
if [[ "$ratio" == "1.10" ]] && ! ls "$d"/opt.in "$d"/Original_PDB.backup >/dev/null 2>&1 \
   && grep -q 'Optimization finished' "$d/autoparams.0000.out"; then pass "-o: optimized geometry used for the job-directory copy"
else fail "-o: optimized geometry used for the job-directory copy" "ratio=$ratio $(ls "$d" | tr '\n' ' ')"; fi

rm -rf "$SCR"
echo "----"; echo "$((N-FAILS))/$N passed"
[[ $FAILS -eq 0 ]]
