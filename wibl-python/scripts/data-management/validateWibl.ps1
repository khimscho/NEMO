param(
  [Parameter(Mandatory=$True, Position=0)]
  [string]$inPath,

  [Parameter(Mandatory=$False)]
  [string]$extension = "json",

  [Parameter(Mandatory=$False)]
  [string]$schema
)

Get-ChildItem -Path $inPath -Filter "*.$extension" -File -Recurse |
  ForEach-Object -Process {
    # Run wibl editwibl to inject metadata
    Write-Output "Validating file $_..."
    if ( $PSBoundParameters.ContainsKey('schema') ) {
        csbschema validate -f $_ --version $schema
    } else {
        csbschema validate -f $_
    }
    if ( -not ($LASTEXITCODE -eq 0) ) {
      Write-Error "Error validating $_, exiting..."
      exit -1
    }
  }
