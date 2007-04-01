// ScanGlue.h - Some wrapper routines around the ScannerBe interface.
//
// Copyright © 1997, Jim Moy

#pragma once

#include <SupportDefs.h>
#include "ScannerBe.h"
class BBitmap;

// This is the one-function "just scan me an image" routine.
BBitmap* GetScannerImage( status_t &status );

// This gives you a bit more flexibility. See the .cpp file.
BBitmap* GetNextScannerImage( scan_id id, status_t &status  );
