# create-signing-cert.ps1
# Creates a self-signed code-signing certificate, exports it as a PFX,
# round-trips it through base64, and verifies the result matches the original.
#
# Usage:
#   .\create-signing-cert.ps1 [-Subject "CN=YourName"] [-Password "secret"] [-OutDir "."]
#
# After a successful run the script prints the base64 string — paste it
# directly into the MSIX_CERTIFICATE_BASE64 GitHub secret.

param(
    [string] $Subject  = "CN=PartVaultDev",
    [string] $Password = "PartVaultDev",
    [string] $OutDir   = $PSScriptRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ── 1. Create self-signed certificate ────────────────────────────────────────
Write-Host "Creating self-signed certificate '$Subject'..."

$cert = New-SelfSignedCertificate `
    -Subject          $Subject `
    -CertStoreLocation "Cert:\CurrentUser\My" `
    -KeyUsage         DigitalSignature `
    -KeyAlgorithm     RSA `
    -KeyLength        2048 `
    -HashAlgorithm    SHA256 `
    -Type             CodeSigningCert `
    -NotAfter         (Get-Date).AddYears(3)

Write-Host "  Thumbprint : $($cert.Thumbprint)"
Write-Host "  Subject    : $($cert.Subject)"
Write-Host "  Expires    : $($cert.NotAfter)"

# ── 2. Export as PFX ─────────────────────────────────────────────────────────
$pfxPath = Join-Path $OutDir "signing-cert.pfx"
$securePass = ConvertTo-SecureString $Password -AsPlainText -Force

Export-PfxCertificate `
    -Cert     "Cert:\CurrentUser\My\$($cert.Thumbprint)" `
    -FilePath $pfxPath `
    -Password $securePass | Out-Null

Write-Host "`nPFX written to: $pfxPath ($((Get-Item $pfxPath).Length) bytes)"

# ── 3. Encode PFX to standard base64 (no line breaks) ────────────────────────
$originalBytes = [IO.File]::ReadAllBytes($pfxPath)
$base64 = [Convert]::ToBase64String($originalBytes)

# Store in a local environment variable (simulates what GitHub Secrets injects)
$env:MSIX_CERTIFICATE_BASE64 = $base64

Write-Host "`nBase64 length  : $($base64.Length) characters"
Write-Host "Length mod 4   : $($base64.Length % 4)  (must be 0)"
Write-Host "`n--- COPY THE FOLLOWING INTO YOUR GITHUB SECRET ---"
Write-Host $base64
Write-Host "---------------------------------------------------`n"

# ── 4. Decode back from the env var ─────────────────────────────────────────
$roundTripBytes = [Convert]::FromBase64String($env:MSIX_CERTIFICATE_BASE64)

$roundTripPath = Join-Path $OutDir "signing-cert-roundtrip.pfx"
[IO.File]::WriteAllBytes($roundTripPath, $roundTripBytes)

# ── 5. Compare with original ─────────────────────────────────────────────────
Write-Host "Comparing files..."

$originalHash   = (Get-FileHash -Path $pfxPath          -Algorithm SHA256).Hash
$roundTripHash  = (Get-FileHash -Path $roundTripPath     -Algorithm SHA256).Hash

Write-Host "  Original   SHA256 : $originalHash"
Write-Host "  Round-trip SHA256 : $roundTripHash"

if ($originalHash -eq $roundTripHash) {
    Write-Host "`n[PASS] Files are identical — base64 round-trip is correct." -ForegroundColor Green
} else {
    Write-Error "[FAIL] Files differ — base64 encoding/decoding produced a different result."
}

# Clean up the round-trip file; keep the real PFX for actual use
Remove-Item $roundTripPath -Force

Write-Host "`nDone. Store '$pfxPath' securely and do not commit it to the repository."
