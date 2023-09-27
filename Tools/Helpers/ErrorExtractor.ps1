<#
    ~ Err.exe error extractor

    This script extracts all error records from 'Err.exe' and
    writes it in a binary DLL that can be used later on by
    'Get-ErrorString'.
    This allows us to easily update our database for new
    versions of 'Err.exe'.

    https://learn.microsoft.com/windows/win32/debug/system-error-code-lookup-tool
#>

$error_obj = C:\Users\francisco.nabas\OneDrive\Tools\Err.exe /:outputtoCSV | ConvertFrom-Csv
$stream = [System.IO.FileStream]::new('C:\LocalRepositories\.WindowsUtils\Tools\Libraries\ErrorLibrary.dll', 'OpenOrCreate', 'ReadWrite', 'Read')
$writer = [System.IO.BinaryWriter]::new($stream)

# Writing the record count in the header.
$writer.Write([uint]$error_obj.Count)
foreach ($record in $error_obj) {

    $writer.Write([convert]::ToInt32($record.HexID, 16))
    $writer.Write($record.SymbolicName)
    $writer.Write($record.Description.Replace('&#10;', "`n").Replace('&apos;', "'"))
}
$writer.Dispose()