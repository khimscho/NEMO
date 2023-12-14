package api

type VersionInfo struct {
	Firmware         string `json:"firmware"`
	CommandProcessor string `json:"commandproc"`
	NMEA0183         string `json:"nmea0183"`
	NMEA2000         string `json:"nmea2000"`
	IMU              string `json:"imu"`
	Serialiser       string `json:"serialiser"`
}

type WebServerInfo struct {
	CurrentStatus string `json:"current"`
	BootStatus    string `json:"boot"`
}

type DataSentence struct {
	Name      string  `json:"name"`
	Tag       string  `json:"tag"`
	Time      float64 `json:"time"`
	TimeUnits string  `json:"time_units"`
	Display   string  `json:"display"`
}

type DataInfo struct {
	Count  uint           `json:"count"`
	Detail []DataSentence `json:"detail"`
}

type DataSummary struct {
	Nmea0183 DataInfo `json:"nmea0183"`
	Nmea2000 DataInfo `json:"nmea2000"`
}

type FileEntry struct {
	Id  uint   `json:"id"`
	Len uint32 `json:"len"`
	MD5 string `json:"md5"`
	Url string `json:"url"`
}

type FileInfo struct {
	Count  uint        `json:"count"`
	Detail []FileEntry `json:"detail"`
}

type Status struct {
	Versions    VersionInfo   `json:"version"`
	Elapsed     uint32        `json:"elapsed"`
	Server      WebServerInfo `json:"webserver"`
	CurrentData DataSummary   `json:"data"`
	Files       FileInfo      `json:"files"`
}

type TransferResult struct {
	Status string `json:"status"`
}
