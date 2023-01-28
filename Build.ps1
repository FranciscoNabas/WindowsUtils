param (
    [switch]$Local,
    [string]$psgak
)

$releaseDir = '.\Release\WindowsUtils'
$copypath = @(
    '.\LICENSE'
    '.\ModuleInfo\WindowsUtils.psd1'
    '.\ModuleInfo\WindowsUtils.psm1'
    '.\ModuleInfo\WindowsUtils.Types.ps1xml'
)

if ($Local) {
    Write-Host Building... -ForegroundColor DarkGreen
    [void](. dotnet build WindowsUtils.csproj --arch x64 --configuration Release --output $releaseDir --no-incremental)
    Write-Host Generating help... -ForegroundColor DarkGreen
    [void](. 'C:\Repositories\NuGet\xmldoc2cmdletdoc\0.3.0\tools\XmlDoc2CmdletDoc.exe' "$releaseDir\WindowsUtils.dll")
}
else {
    Write-Host Building... -ForegroundColor DarkGreen
    . dotnet build WindowsUtils.csproj --arch x64 --configuration Release --output $releaseDir --no-incremental
}

Write-Host 'Cleaning files...' -ForegroundColor DarkGreen
'*.config', '*.pdb', '*.json', 'WindowsUtils.xml', 'Core.exp', 'Core.lib', 'Core.dll.metagen' | ForEach-Object { Remove-Item -Path "$releaseDir\*" -Filter $PSItem -Force }

Write-Host 'Copying files...' -ForegroundColor DarkGreen
Copy-Item -Path $copypath -Destination $releaseDir -Force

Write-Host 'Copying CRT libraries...' -ForegroundColor DarkGreen
Copy-Item -Path '.\StaticDependencies\bin' -Destination $releaseDir -Recurse -Force -Exclude 'Core*' 

if (!(Test-Path "$releaseDir\en-us")) { [void](mkdir "$releaseDir\en-us") }
Move-Item "$releaseDir\WindowsUtils.dll-Help.xml" "$releaseDir\en-us\WindowsUtils.dll-Help.xml" -Force

if (!$Local) { Write-Host 'Compressing files...' -ForegroundColor DarkGreen; Compress-Archive -Path "$releaseDir\*" -DestinationPath .\WindowsUtils.zip }

if ($Local) {
    if ([string]::IsNullOrEmpty($psgak)) { Write-Warning "psgak is null" }
    else {
        Write-Host 'Publishing module...' -ForegroundColor DarkGreen
        try {
            $mdata = Test-ModuleManifest "$releaseDir\WindowsUtils.psd1" -ErrorAction Stop
            Write-Host "Manifest version: $($mdata.Version.ToString())" -ForegroundColor DarkGreen
            Start-Sleep 7
            Publish-Module -Path $releaseDir -NuGetApiKey $psgak -Repository PSGallery
        }
        catch { Write-Host "Failed to publish module. $($PSItem.Exception.Message)" -ForegroundColor Yellow }            
    }
}