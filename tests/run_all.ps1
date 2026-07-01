$ErrorActionPreference = "Continue"

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build\pda_cfg.exe"
$outputDir = Join-Path $PSScriptRoot "output"

if (-not (Test-Path $exe)) {
    throw "Executable not found: $exe"
}

New-Item -ItemType Directory -Force $outputDir | Out-Null

$tests = @(
    @{ Name = "cfg_sample"; Mode = "cfg"; Input = "cfg_sample.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_no_epsilon"; Mode = "cfg"; Input = "cfg_no_epsilon.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_nullable_start"; Mode = "cfg"; Input = "cfg_nullable_start.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_unit_chain"; Mode = "cfg"; Input = "cfg_unit_chain.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_unreachable"; Mode = "cfg"; Input = "cfg_unreachable.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_non_generating"; Mode = "cfg"; Input = "cfg_non_generating.txt"; ExpectedExit = 0 },
    @{ Name = "cfg_compact_sample"; Mode = "cfg"; Input = "cfg_compact_sample.txt"; ExpectedExit = 0 },
    @{ Name = "pda_sample"; Mode = "pda"; Input = "pda_sample.txt"; ExpectedExit = 0 },
    @{ Name = "pda_direct_pop"; Mode = "pda"; Input = "pda_direct_pop.txt"; ExpectedExit = 0 },
    @{ Name = "pda_empty_input"; Mode = "pda"; Input = "pda_empty_input.txt"; ExpectedExit = 0 },
    @{ Name = "pda_push_three"; Mode = "pda"; Input = "pda_push_three.txt"; ExpectedExit = 0 },
    @{ Name = "err_cfg_undeclared_symbol"; Mode = "cfg"; Input = "err_cfg_undeclared_symbol.txt"; ExpectedExit = 1 },
    @{ Name = "err_cfg_reserved_terminal"; Mode = "cfg"; Input = "err_cfg_reserved_terminal.txt"; ExpectedExit = 1 },
    @{ Name = "err_pda_undeclared_state"; Mode = "pda"; Input = "err_pda_undeclared_state.txt"; ExpectedExit = 1 },
    @{ Name = "err_pda_reserved_stack_symbol"; Mode = "pda"; Input = "err_pda_reserved_stack_symbol.txt"; ExpectedExit = 1 },
    @{ Name = "err_pda_missing_push"; Mode = "pda"; Input = "err_pda_missing_push.txt"; ExpectedExit = 1 }
)

$failed = 0

foreach ($test in $tests) {
    $inputPath = Join-Path $PSScriptRoot $test.Input
    $outputPath = Join-Path $outputDir ($test.Name + ".out.txt")

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $exe
    $startInfo.Arguments = $test.Mode
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.CreateNoWindow = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    [void]$process.Start()
    $process.StandardInput.Write((Get-Content -Path $inputPath -Raw))
    $process.StandardInput.Close()
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    $exitCode = $process.ExitCode

    ($stdout + $stderr) | Out-File -FilePath $outputPath -Encoding UTF8

    if ($exitCode -eq $test.ExpectedExit) {
        Write-Host ("PASS {0} exit={1}" -f $test.Name, $exitCode)
    } else {
        Write-Host ("FAIL {0} exit={1}, expected={2}" -f $test.Name, $exitCode, $test.ExpectedExit)
        $failed++
    }
}

if ($failed -ne 0) {
    exit 1
}

exit 0
