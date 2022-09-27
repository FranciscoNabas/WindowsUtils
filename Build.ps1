param ($psgsec)

## Artifact output path
$releaseDir = '.\bin\WindowsUtils'

## Building project
. dotnet build WindowsUtils.csproj --arch x64 --configuration Release --output bin\WindowsUtils --no-incremental

## Removing files
'*.config', '*.pdb', '*.json' | ForEach-Object { Remove-Item -Path "$releaseDir\*" -Filter $PSItem -Force }

## Copying license and compressing output
Write-Output 'Copying license...'
Copy-Item -Path .\LICENSE -Destination $releaseDir -Force
Write-Output 'Compressing artifact...'
Compress-Archive -Path "$releaseDir\*" -DestinationPath .\WindowsUtils.zip

## Publishing module
#try {
#    Write-Output 'Trying to publish module...'
#    Publish-Module -Path $releaseDir -NuGetApiKey $psgsec -Repository PSGallery
#}
#catch { Write-Output "Failed to publish module. $($PSItem.Exception.Message)" }