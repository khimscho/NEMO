/*!\file ProcessingManager.cpp
 * \brief Keep track of processing steps the logger wants to recommend.
 *
 * This code implements a mechanism to hold informtion on processing that the logger would
 * like the cloud processing to implement (e.g., de-duplication of depths, spike removal, etc.)
 * The cloud would have to keep track of every logger in order to know what to do, which would
 * be expensive; this way, the logger can tell you what you need to be doing (desides timestamping
 * and converting to the right format for submission to the database).
 *
 * Copyright (c) 2021, University of New Hampshire, Center for Coastal and Ocean Mapping.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ProcessingManager.h"
#include "LogManager.h"
#include "SPIFFS.h"

namespace logger {

ProcessingManager::ProcessingManager(void)
{
    m_algorithms = "/algorithms.txt";
    m_parameters = "/parameters.txt";
}

ProcessingManager::~ProcessingManager(void)
{
}

void ProcessingManager::AddAlgorithm(String const& alg_name, String const& alg_params)
{
    File alg = SPIFFS.open(m_algorithms, FILE_APPEND);
    alg.println(alg_name);
    alg.close();
    File param = SPIFFS.open(m_parameters, FILE_APPEND);
    param.println(alg_params);
    param.close();
}

void ProcessingManager::ListAlgorithms(Stream& s)
{
    File alg = SPIFFS.open(m_algorithms, FILE_READ);
    File params = SPIFFS.open(m_parameters, FILE_READ);

    while (alg.available()) {
        String algorithm = alg.readString();
        String parameters = params.readString();
        s.printf("alg = \"%s\", params = \"%s\"\n", algorithm.c_str(), parameters.c_str());
    }
    alg.close();
    params.close();
}

void ProcessingManager::SerialiseAlgorithms(Serialiser *s)
{
    File alg = SPIFFS.open(m_algorithms, FILE_READ);
    File params = SPIFFS.open(m_parameters, FILE_READ);

    while (alg.available()) {
        Serialisable ser(255);
        String algorithm = alg.readString();
        String parameters = params.readString();
        ser += algorithm.length();
        ser += algorithm.c_str();
        ser += parameters.length();
        ser += parameters.c_str();
        s->Process(logger::Manager::PacketIDs::Pkt_Algorithms, ser);
    }
}

}
