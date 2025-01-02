Push-Location -Path ([System.IO.Path]::Combine($PSScriptRoot, '..\..'))

Remove-Item -Path '.\Release\WindowsUtils' -Recurse -Force
$releaseDir = New-Item -Path '.\Release' -Name 'WindowsUtils' -ItemType Directory
[void][System.IO.Directory]::CreateDirectory('C:\LocalRepositories\.WindowsUtils\Release\WindowsUtils\en-us')
$copyPath = @(
    '.\LICENSE'
    '.\Tools\Metadata\WindowsUtils.psd1'
    '.\Tools\Metadata\WindowsUtils.psm1'
    '.\Tools\Metadata\WindowsUtils.Types.ps1xml'
    '.\Tools\Metadata\WindowsUtils.Format.ps1xml'
    '.\src\bin\x64\Release\netstandard2.0\*'
    '.\Tools\Libraries\ErrorLibrary.dll'
)

Copy-Item -Path $copyPath -Destination $releaseDir.FullName -Force
Copy-Item -Path '.\Tools\Libraries\win-x64\msvcp140.dll', '.\Tools\Libraries\win-x64\vcruntime140.dll', '.\Tools\Libraries\win-x64\vcruntime140_1.dll' -Destination $releaseDir.FullName -Recurse -Force
Move-Item ([System.IO.Path]::Combine($releaseDir.FullName, 'WindowsUtils.dll-Help.xml')) "$($releaseDir.FullName)\en-us\WindowsUtils.dll-Help.xml" -Force
'*.config', '*.json', 'WindowsUtils.xml', 'Core.exp', 'Core.lib', 'Core.dll.metagen' | ForEach-Object { Remove-Item -Path "$($releaseDir.FullName)\*" -Filter $_ -Force }

Pop-Location