/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPacketFileReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPacketFileReader -
// .SECTION Description
//

#ifndef __vtkPacketFileReader_h
#define __vtkPacketFileReader_h

#include <pcap.h>
#include <string>

class vtkPacketFileReader
{
public:

  vtkPacketFileReader()
  {
    this->PCAPFile = 0;
  }

  ~vtkPacketFileReader()
  {
    this->Close();
  }

  bool Open(const std::string& filename)
  {
    char errbuff[PCAP_ERRBUF_SIZE];
    pcap_t *pcapFile = pcap_open_offline(filename.c_str (), errbuff);
    if (!pcapFile)
      {
      this->LastError = errbuff;
      return false;
      }

    struct bpf_program filter;

    // PCAP_NETMASK_UNKNOWN should be 0xffffffff, but it's undefined in older PCAP versions
    if (pcap_compile(pcapFile, &filter, "udp ", 0, 0xffffffff) == -1)
      {
      this->LastError = pcap_geterr(pcapFile);
      return false;
      }
    else if (pcap_setfilter(pcapFile, &filter) == -1)
      {
      this->LastError = pcap_geterr(pcapFile);
      return false;
      }

    this->FileName = filename;
    this->PCAPFile = pcapFile;
    this->StartTime.tv_sec = this->StartTime.tv_usec = 0;
    return true;
  }

  bool IsOpen()
  {
    return (this->PCAPFile != 0);
  }

  void Close()
  {
    if (this->PCAPFile)
      {
      pcap_close(this->PCAPFile);
      this->PCAPFile = 0;
      this->FileName.clear();
      }
  }

  const std::string& GetLastError()
  {
    return this->LastError;
  }

  const std::string& GetFileName()
  {
    return this->FileName;
  }


  bool NextPacket(const unsigned char*& data, unsigned int& dataLength, double& timeSinceStart)
  {
    if (!this->PCAPFile)
      {
      return false;
      }

    struct pcap_pkthdr *header;
    int returnValue = pcap_next_ex(this->PCAPFile, &header, &data);
    if (returnValue < 0)
      {
      this->Close();
      return false;
      }

    // The ethernet header is 42 bytes long; unnecessary
    const unsigned int bytesToSkip = 42;
    dataLength = header->len - bytesToSkip;
    data = data + bytesToSkip;
    timeSinceStart = GetElapsedTime(header->ts, this->StartTime);
    return true;
  }

protected:

  double GetElapsedTime(const struct timeval& end, const struct timeval& start)
  {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.00;
  }

  pcap_t* PCAPFile;
  std::string FileName;
  std::string LastError;
  struct timeval StartTime;
};

#endif