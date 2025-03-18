#pragma once

int LoadGeoTarget(const char* fn);
int FindGeoTarget(unsigned int ip); // ip has host  byte order
int FindGeoTarget(const char* ipStr);
