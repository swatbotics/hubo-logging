#include "LogReader.h"
#include <stdio.h>
#include <stdexcept>
#include <assert.h>
#include <iostream>

LogReader::LogReader(): 
  _frequency(0), _numTicks(0) {}

LogReader::LogReader(const std::string& filename):
  _frequency(0), _numTicks(0) {

  open(filename);

}

void LogReader::open(const std::string& filename) {

  _names.clear();
  _units.clear();
  _data.clear();

  FILE* fp = fopen(filename.c_str(), "rb");
  if (!fp) { 
    throw std::runtime_error("Error opening " + filename);
  }

  int total_n_numbers, n_channels, n_points;
  
  if (fscanf(fp, "%d%d%d%f", 
	     &total_n_numbers, 
	     &n_channels,
	     &n_points,
	     &_frequency) != 4) {
    throw std::runtime_error("error reading header");
  }

  if (total_n_numbers != n_channels * n_points) {
    throw std::runtime_error("failed sanity check n_channels * n_points");
  }

  bool incomplete = (total_n_numbers == 0);

  _numTicks = n_points;

  char name[1024], unit[1024];

  for (int i=0; i<n_channels; ++i) {
    if (fscanf(fp, "%1023s%1023s", name, unit) != 2) {
      throw std::runtime_error("error reading name/units");
    }
    _names.push_back(name);
    _units.push_back(unit);
  }

  fscanf(fp, "%c%c%c", name, name, name);
  
  std::vector<char> rowbuffer;
  std::vector<float> rowmajor;

  rowbuffer.resize( 4 * n_channels );
  
  int n_rows_read = 0;

  while (1) {
    size_t nread = fread(&(rowbuffer[0]), rowbuffer.size(), 1, fp);
    if (nread == 0) {
      if (incomplete || n_rows_read == n_points) {
	break;
      } else {
	throw std::runtime_error("error reading float data");
      }
    }
    ++n_rows_read;
    const char* src = &(rowbuffer[0]);
    for (int i=0; i<n_channels; ++i) {
      float f;
      char* c = (char*)&f;
      for (int j=0; j<4; ++j) {
	c[3-j] = *src++;
      }
      rowmajor.push_back(f);
    }
  }

  if (incomplete) { 
    _numTicks = n_points = n_rows_read;
    total_n_numbers = n_rows_read * n_channels;
    std::cerr << "warning: log file was incomplete, found " << _numTicks << " samples.\n";
  }

  assert( int(rowmajor.size()) == total_n_numbers );
  
  fclose(fp);

  // now transpose data
  _data.resize(total_n_numbers);

  for (int i=0; i<n_channels; ++i) {
    for (int j=0; j<n_points; ++j) {
      _data[i * n_points + j] = rowmajor[j * n_channels + i];
    }
  }


}

float LogReader::frequency() const { 
  return _frequency;
}

size_t LogReader::numChannels() const {
  return _names.size();
}

size_t LogReader::numTicks() const {
  return _numTicks;
}

const std::string& LogReader::channelName(size_t i) const {
  return _names[i];
}

const std::string& LogReader::channelUnits(size_t i) const {
  return _units[i];
}

size_t LogReader::lookupChannel(const std::string& name) const {
  for (size_t i=0; i<_names.size(); ++i) {
    if (_names[i] == name) { return i; }
  }
  return npos;
}

float LogReader::operator()(size_t channel, size_t tick) const {
  return _data[channel * _numTicks + tick];
}

const float* LogReader::channelData(size_t i) const {
  return &(_data[i * _numTicks]);
}
