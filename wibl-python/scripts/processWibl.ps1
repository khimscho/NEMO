param(
  [string]$wiblPath,
  [string]$metadataFile,
  [string]$wiblConfig
)

Get-ChildItem -Path $wiblPath -Filter *.wibl -File -Recurse |
  ForEach-Object -Process {
    Write-Output "Input file name: $_"
    # Run wibl editwibl to inject metadata
    $editedName = $_ -replace '\.wibl', '-edited.wibl'
    Write-Output "Edited file name: $editedName"
    Write-Output "Running command: 'wibl editwibl -m $metadataFile $_ $editedName'..."
    #wibl editwibl -m $metadataFile $_ $editedName
    # Run wibl procwibl to create GeoJSON output
    $processedName = $_ -replace '\.wibl', '.geojson'
    Write-Output "GeoJSON file name: $processedName"
    Write-Output "Running command: 'wibl procwibl -c $wiblConfig $editedName $processedName'..."
    #wibl procwibl -c $wiblConfig $editedName $processedName
  }
