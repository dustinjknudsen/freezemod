cat > /data/projects/freezemod/CLAUDE.md << 'EOF'
# Freezemod Build Workflow

## Build commands
- Build: `make 2>&1 | tail -20`
- Install (deploy DLL to both Steam paths): `make install`
- Force rebuild if `make` says "nothing to do": `touch src/<file> && make`

## Verification
After every code change, BEFORE telling user the fix is ready:
1. Run `make` and check it actually compiled (look for "==> build/game_x64.dll" line)
2. Run `make install` and confirm both deploy paths show
3. Compare timestamps: `ls -la build/game_x64.dll src/<edited_file>`
4. The DLL must be NEWER than the source file

## When user reports a fix isn't working
FIRST CHECK: Was the DLL actually rebuilt and deployed? Compare timestamps
before assuming the source fix is wrong. Most "fix not working" reports are
actually "the DLL is stale."

## Project doesn't use Visual Studio
The MuffMode.sln file exists but is unused. Build via Makefile + MinGW only.
EOF
