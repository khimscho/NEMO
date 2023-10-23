<#
.DESCRIPTION
This applet converts .DAT files in a .zip file to .WIBL files in a directory that matches the source .zip.

.OUTPUTS
A directory structure containing .WIBL files matching the input .zip file

.INPUTS
A .zip file containing subdirectories of .DAT files

.NOTES
The -verbose flag will show you some more information of what's happening behind the scenes.
The -debug flag will tell you about each file as they're getting converted

This script is multithreaded, and therefore requires at least Powershell 7.0.0
This was developed using Powershell 7.3.4

Developed by Chris Schwartz at The Center for Coastal and Ocean Mapping, 7-19-2023.
cschwartz@ccom.unh.edu / christopher.schwartz@unh.edu

.PARAMETER Source
The source .ZIP file to extract the .DAT files from

.PARAMETER LogConvertPath
The location of logconvert.exe, if it isn't in a folder next to the script called "logconvert"

.PARAMETER OutputFolder
A folder to output all of the processed files to. This folder will have the same structure as the
source zip folder

.PARAMETER ShowLogConvertOutput
This will print out the logconvert.exe program's output to the command line.

.EXAMPLE
PS > ./Convert-DATToWIBL.ps1 input.zip output

.EXAMPLE
PS > ./Convert-DATToWIBL.ps1 -Source input.zip -OutputFolder C:\foo\bar\baz

.EXAMPLE
PS > ./Convert-DATToWIBL -Source input.zip -OutputFolder C:\foo\bar\baz -LogConvertPath C:\foo\bar\logconvert.exe

.EXAMPLE
PS > ./Convert-DATToWIBL -Source input.zip -OutputFolder C:\foo\bar\baz -LogConvertPath C:\foo\bar\logconvert.exe

#>

#Requires -Version 7

[CmdletBinding()]

param (
    # The input ZIP file    
    [Parameter( Mandatory = $True, Position = 0, ValueFromPipeline )]
    [string]$Source,
    
    # The output to send the resulting .WIBL files
    [Parameter( Mandatory = $True, Position = 1  )]
    [string]$OutputFolder,

    # The logconvert program can be anywhere, but we can assume it
    # to be in the local directory if the user doesn't specifiy it.
    [string]$LogConvertPath = ".\logconvert\logconvert.exe",

    # A switch to control whether or not the logconvert program will
    # print stdout to the command line.
    [switch]$ShowLogConvertOutput
)

# Before starting, verify that we have access to our logconvert program.
# This should be in a folder on the same level as the script
if ( -not ( Test-Path -Path $LogConvertPath ) ) {
    Write-Error "Could not find logconvert. Make sure it's in a folder in the same directory as this script."
    exit -1
}

# Otherwise we may be good to go. We're going to assume logconvert has all of it's dependencies
# in the directory with it. 

# Make sure the input zip exists. This is more of a sanity check.
if ( -not ( Test-Path "$Source" ) ) {
    Write-Error "Source ZIP file does not exist. Check your path and try again."
    exit -2
}
elseif ( -not ( $Source.ToLower().EndsWith( ".zip" ) ) ) {
    Write-Error "Source is not a ZIP file!"
    exit -3
}

# And now we should make sure the output directory exists.
if ( -not ( Test-Path -Path $OutputFolder ) ) {
    Write-Verbose "Output directory not found. Creating a new directory."
    New-Item -Path $OutputFolder -Name $OutputFolderName -ItemType Directory | Out-Null
}
else {
    # TODO ask if they want to overwrite the already existing directory
    $OverwriteOutputResponse = Read-Host -Prompt "Output directory exists. Do you want to overwrite it? ( y\n )"
    if ( $OverwriteOutputResponse.ToLower() -ne "y" ) {
        exit 0
    }
    else {
        Remove-Item -Recurse -Force -Path $OutputFolder
    }
}

# And now, lettuce begin.

# Set a path for the expanded archive
$ExpandedArchivePath = "$( $Env:TEMP )\temp_dat_to_wibl_output"

# First things first, get the source zip and unzip it to a temporary location
# This will overwrite a folder named temp_dat_output if it finds one.
Expand-Archive -Path $Source -DestinationPath $ExpandedArchivePath -Force | Out-Null

# Get the folder structure of the resulting archive
$OutputFromZIPFolder = Get-ChildItem -Path $ExpandedArchivePath -Directory
$DirectoryStructureToCopy = Get-ChildItem -Path $OutputFromZIPFolder -Directory -Recurse

# And mirror it to the output directory
Write-Verbose "Creating directory in output folder"
$DirectoryStructureToCopy | ForEach-Object {
    New-Item -Path $OutputFolder -Name $_.Name -ItemType Directory | Out-Null
}

# Now that we have that, go through the files in the
# initial unzipped directory and convert each one to a .WIBL file
Write-Verbose "Getting list of .DAT files"
$AllDATFiles = Get-ChildItem -Path $OutputFromZIPFolder -Filter "*.dat" -Recurse

# A quick variable to get the date right as we started
$TimeThen = Get-Date

$AllDATFiles | Foreach-Object -ThrottleLimit 20 -Parallel {
    [CmdletBinding()]

    # Get the name of the folder to copy to
    # This is the direct parent directory of this item in the
    # extracted .ZIP
    $SourceZIPSubdirectoryFileOriginatesFrom = $PSItem.DirectoryName.Split( "\" )[ -1 ]
    $DirectoryToCopyTo = "$( $USING:OutputFolder )\$( $SourceZIPSubdirectoryFileOriginatesFrom )"

    # This is more for clarity of what Name vs. Basename is.
    # Instead of getting foo.bar, it just returns foo.
    $FileNameNoExtension = $PSItem.BaseName
    $InputFilePath = "$( $USING:OutputFromZIPFolder )\$( $SourceZIPSubdirectoryFileOriginatesFrom )\$( $PSItem.Name )"
    $OutputFilePath = "$( $DirectoryToCopyTo )\$( $FileNameNoExtension ).wibl"

    # Output a bunch of verbose output just in case we want it.
    Write-Verbose "Processing $( $PSItem.Name )..." -Verbose
    Write-Debug "---" -Debug
    Write-Debug "File name: $( $PSItem.Name )" -Debug
    Write-Debug "Source path: $( $InputFilePath )" -Debug
    Write-Debug "Target path: $( $OutputFilePath )" -Debug

    Write-Debug "Starting conversion..." -Debug
    # Start the logconvert program, and use the input from the extracted zip.
    # Then, tell the output to use the OutputFolder \ Subdirectory this file is in \ foo.wibl
    Write-Debug "Calling logconvert.exe with arguments: -f YDVR -i '$( $InputFilePath )' -o '$( $OutputFilePath )'" -Debug
    If ( $ShowLogConvertOutput ) {
        & $USING:LogConvertPath -f YDVR -i "$( $InputFilePath )" -o "$( $OutputFilePath )"
    }
    Else {
        & $USING:LogConvertPath -f YDVR -i "$( $InputFilePath )" -o "$( $OutputFilePath )" | Out-Null
    }

    # Continue on with the next one
}

# And then another for when we finish processing
$TimeNow = Get-Date

# Compute the difference between the two times
$TimeBetween = $TimeNow - $TimeThen
$SecondsPassed = ( $TimeBetween.Seconds * 1000 )

# And print out the runtime if we have the debug or verbose flags set
Write-Debug "Took $( $TimeBetween.Milliseconds + $SecondsPassed )ms"
Write-Verbose "Took $( $TimeBetween.Milliseconds + $SecondsPassed )ms"

# And now the cleanup
Write-Verbose "Cleaning up temporary files"
Remove-Item $ExpandedArchivePath -Recurse -Force | Out-Null

Write-Debug "Done!"
Write-Verbose "Done!"