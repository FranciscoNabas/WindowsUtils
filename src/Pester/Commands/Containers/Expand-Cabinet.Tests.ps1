BeforeAll {
    function Test-ExpandCabOutputMetadata {

        [CmdletBinding()]
        param (
            [Parameter(Mandatory)]
            [ValidateScript({
                if (![System.IO.Directory]::Exists($_)) {
                    throw [System.IO.FileNotFoundException]::new("Could not find the directory '$_'.")
                }

                return $true
            })]
            [string]$Destination,

            [Parameter(Mandatory)]
            [ValidateNotNullOrEmpty()]
            [object[]]$Metadata
        )

        $compliant = $true
        foreach ($file in (Get-ChildItem -Path $Destination -Recurse -Force -File)) {
            $currentData = $Metadata | Where-Object -FilterScript { $_.Name -eq $file.Name }
            if ($currentData) {
                if ($currentData.Length -eq $file.Length -and $currentData.Attributes -eq $file.Attributes -and
                    $currentData.Sha256Hash -eq (Get-FileHash -Path $file.FullName -Algorithm SHA256).Hash) {

                    $compliant = $compliant -band $true
                    continue
                }
            }

            $compliant = $compliant -band $false
        }

        return $compliant
    }

    $utilitiesRoot = Get-ItemProperty -Path "$PSScriptRoot\..\..\Utilities"
    $Global:cabPath = Get-ChildItem -Path ([System.IO.Path]::Combine($utilitiesRoot.FullName, 'PesterTestCab02.cab')) -ErrorAction Stop
    $metadataPath = [System.IO.Path]::Combine($utilitiesRoot.FullName, 'PesterTestCabMetadata.json')

    if (![System.IO.File]::Exists($metadataPath)) {
        throw [System.IO.FileNotFoundException]::new("Could not find the cabinet metadata file '$metadataPath'.")
    }

    $Global:tempFolderInfo     = [System.IO.Directory]::CreateDirectory([System.IO.Path]::Combine($utilitiesRoot.FullName, "ExpandoCabinetoTemp-$([DateTime]::Now.ToString('yyyyMMdd-HHmmss'))"))
    $Global:cabMetadata        = Get-Content -Path $metadataPath -Raw | ConvertFrom-Json
    $Global:globbedTempFolder  = [System.IO.Path]::Combine($Global:tempFolderInfo.FullName, '*')
}

Describe 'Expand-Cabinet' {
    It "Expand one or more cabinet files with explicit 'Path' parameter" {
        Expand-Cabinet -Path $Global:cabPath.FullName -Destination $Global:tempFolderInfo.FullName
        Test-ExpandCabOutputMetadata -Destination $Global:tempFolderInfo.FullName -Metadata $Global:cabMetadata | Should -Be $true
        Remove-Item -Path $Global:globbedTempFolder -Force
    }

    It "Expand one or more cabinet files with explicit 'LiteralPath' parameter" {
        Expand-Cabinet -LiteralPath $Global:cabPath.PSPath -Destination $Global:tempFolderInfo.FullName
        Test-ExpandCabOutputMetadata -Destination $Global:tempFolderInfo.FullName -Metadata $Global:cabMetadata | Should -Be $true
        Remove-Item -Path $Global:globbedTempFolder -Force
    }

    It 'Expand one or more cabinet files with value from the pipeline' {
        Get-ChildItem -Path $Global:cabPath.FullName | Expand-Cabinet -Destination $Global:tempFolderInfo.FullName
        Test-ExpandCabOutputMetadata -Destination $Global:tempFolderInfo.FullName -Metadata $Global:cabMetadata | Should -Be $true
        Remove-Item -Path $Global:globbedTempFolder -Force
    }
}

AfterAll {
    Remove-Item -Path $Global:tempFolderInfo.FullName -Recurse -Force
}