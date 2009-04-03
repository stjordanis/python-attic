/*
 * opencog/embodiment/Control/MessagingSystem/OpcTester.cc
 *
 * Copyright (C) 2007-2008 TO_COMPLETE
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "OPC.h"
#include <SystemParameters.h>
#include <StringMessage.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "util/Logger.h"

using namespace std;

using namespace OperationalPetController;

int main()
{
    Control::SystemParameters sp;
    MessagingSystem::StringMessage *msg = new MessagingSystem::StringMessage("", "", "");
    int length;
    char * buffer;

    ifstream is;
    is.open ("arquivo.xml", ios::binary );

    // get length of file:
    is.seekg (0, ios::end);
    length = is.tellg();
    is.seekg (0, ios::beg);


    // allocate memory:
    buffer = new char [length];

    // read data as a block:
    is.read (buffer, length);
    is.close();
    string xml = buffer;

    //char * file = "arquivo.xml";
    //string xml = "<?xml version=\"1.0\"?><instruction pet-id=\"1\" avatar-id=\"2\" timestamp=\"2007-06-20\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:SchemaLocation=\"http://www.electricsheepcompany.com/xml/ns/PetProxy /home/dlopes/projetos/petaverse/trunk/Petaverse/build/src/PetController/petBrain.xsd\">start learning!</instruction><instruction pet-id=\"1\" avatar-id=\"2\" timestamp=\"2007-06-20\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:SchemaLocation=\"http://www.electricsheepcompany.com/xml/ns/PetProxy /home/dlopes/projetos/petaverse/trunk/Petaverse/build/src/PetController/petBrain.xsd\">stop learning</instruction>";

    server(OPC::createInstance);
    OPC& opc = static_cast<OPC&>(server());
    opc.init("teste-opc", "127.0.0.1", 4000, "1", "2", "pet", "neutral", sp);
    msg->setMessage(xml);
    opc.processNextMessage(msg);

    delete[] buffer;
    // TODO: how to delete opc now?
    //delete opc;
    return 0;
}

