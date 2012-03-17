###############################################################################
#                                                                             #
# This file is part of IfcOpenShell.                                          #
#                                                                             #
# IfcOpenShell is free software: you can redistribute it and/or modify        #
# it under the terms of the Lesser GNU General Public License as published by #
# the Free Software Foundation, either version 3.0 of the License, or         #
# (at your option) any later version.                                         #
#                                                                             #
# IfcOpenShell is distributed in the hope that it will be useful,             #
# but WITHOUT ANY WARRANTY; without even the implied warranty of              #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                #
# Lesser GNU General Public License for more details.                         #
#                                                                             #
# You should have received a copy of the Lesser GNU General Public License    #
# along with this program. If not, see <http://www.gnu.org/licenses/>.        #
#                                                                             #
###############################################################################

###############################################################################
#                                                                             #
# This files uses the documentation files from buildingSMART to generate      #
# descriptions from EXPRESS names that are suitable for comments in the C++   #
# code. The .csv files used by this file are generated from the MS Office     #
# Access database, which in turn has been generated from the IFC baseline     #
# documentation by the IFCDOC utility provided by buildingSMART.              #
#                                                                             #
###############################################################################

import re,csv
import csv
try: from html.entities import entitydefs
except: from htmlentitydefs import entitydefs

name_to_oid = {}
oid_to_desc = {}
oid_to_name = {}
oid_to_pid = {}
regices = list(zip([re.compile(s,re.M) for s in [r'<[\w\n=" \-/\.;_\t:%#,\?\(\)]+>',r'(\n[\t ]*){2,}',r'^[\t ]+','^']],['','\n\n','  ','/// ']))

definition_files = ['DocEntity.csv', 'DocEnumeration.csv', 'DocDefined.csv', 'DocSelect.csv']
for fn in definition_files:
    with open(fn) as f:
        for oid, name, desc in csv.reader(f, delimiter=';', quotechar='"'):
            name_to_oid[name] = oid
            oid_to_name[oid] = name
            oid_to_desc[oid] = desc

with open('DocEntityAttributes.csv') as f:
    for pid, x, oid in csv.reader(f, delimiter=';', quotechar='"'):
        oid_to_pid[oid] = pid
        
with open('DocAttribute.csv') as f:
    for oid, name, desc in csv.reader(f, delimiter=';', quotechar='"'):
        pid = oid_to_pid[oid]
        pname = oid_to_name[pid]
        name_to_oid[(pname, name)] = oid
        oid_to_desc[oid] = desc
        
def description(item):
    global name_to_oid, oid_to_desc, oid_to_name, oid_to_pid
    oid = name_to_oid.get(item,0)
    desc = oid_to_desc.get(oid,None)
    if desc:
        for a,b in entitydefs.items(): desc = desc.replace("&%s;"%a,b)
        desc = desc.replace("\r","")
        for r,s in regices[:-1]: desc = r.sub(s,desc)
        desc = desc.strip()
        r,s = regices[-1]
        desc = r.sub(s,desc)
    return desc