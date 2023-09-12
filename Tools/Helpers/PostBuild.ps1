Set-Location -Path C:\LocalRepositories\.WindowsUtils
$releaseDir = '.\Release\WindowsUtils'
$copypath = @(
    '.\LICENSE'
    '.\Tools\Metadata\WindowsUtils.psd1'
    '.\Tools\Metadata\WindowsUtils.psm1'
    '.\Tools\Metadata\WindowsUtils.Types.ps1xml'
    '.\Tools\Metadata\WindowsUtils.Format.ps1xml'
    '.\src\bin\x64\Release\netstandard2.0\*'
)

Copy-Item -Path $copypath -Destination $releaseDir -Force
Copy-Item -Path '.\Tools\Libraries\win-x64\msvcp140.dll', '.\Tools\Libraries\win-x64\vcruntime140.dll', '.\Tools\Libraries\win-x64\vcruntime140_1.dll' -Destination $releaseDir -Recurse -Force
Move-Item "$releaseDir\WindowsUtils.dll-Help.xml" "$releaseDir\en-us\WindowsUtils.dll-Help.xml" -Force
'*.config', '*.pdb', '*.json', 'WindowsUtils.xml', 'Core.exp', 'Core.lib', 'Core.dll.metagen' | ForEach-Object { Remove-Item -Path "$releaseDir\*" -Filter $PSItem -Force }

