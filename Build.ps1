param (
    [switch]$Local,
    [string]$psgak
)

$releaseDir = '.\bin\WindowsUtils'

Write-Host Building... -ForegroundColor DarkGreen
[void](. dotnet build WindowsUtils.csproj --arch x64 --configuration Release --output bin\WindowsUtils --no-incremental)

if ($Local) {
    Write-Host Generating help... -ForegroundColor DarkGreen
    [void](. 'C:\Repositories\NuGet\xmldoc2cmdletdoc\0.3.0\tools\XmlDoc2CmdletDoc.exe' "$releaseDir\WindowsUtils.dll")
}

Write-Host 'Cleaning files...' -ForegroundColor DarkGreen
'*.config', '*.pdb', '*.json', 'WindowsUtils.xml' | ForEach-Object { Remove-Item -Path "$releaseDir\*" -Filter $PSItem -Force }

Write-Host 'Copying files...' -ForegroundColor DarkGreen
Copy-Item -Path .\LICENSE -Destination $releaseDir -Force
if (!(Test-Path "$releaseDir\en-us")) { [void](mkdir "$releaseDir\en-us") }
Move-Item "$releaseDir\WindowsUtils.dll-Help.xml" "$releaseDir\en-us\WindowsUtils.dll-Help.xml" -Force

if (!$Local) { Write-Host 'Compressing files...' -ForegroundColor DarkGreen; Compress-Archive -Path "$releaseDir\*" -DestinationPath .\WindowsUtils.zip }

if ($Local) {
    if ([string]::IsNullOrEmpty($psgak)) { Write-Warning "psgak is null" }
    else {
        Write-Host 'Publishing module...' -ForegroundColor DarkGreen
        try {
            $mdata = Test-ModuleManifest "$releaseDir\WindowsUtils.psd1" -ErrorAction Stop
            Write-Output $mdata.Version
            Start-Sleep 7
            Publish-Module -Path $releaseDir -NuGetApiKey $psgak -Repository PSGallery
        }
        catch { Write-Output "Failed to publish module. $($PSItem.Exception.Message)" }            
    }
}