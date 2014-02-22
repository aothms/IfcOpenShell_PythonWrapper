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

import templates

class RuntimeTypingImplementation:
    def __init__(self, mapping):
        schema_name = mapping.schema.name.capitalize()
        
        entity_descriptors = []
        enumeration_descriptors = []
        derived_field_statements = []
        
        for name, type in mapping.schema.simpletypes.items():
            entity_descriptors.append(templates.entity_descriptor % {
                'type'                         : name,
                'parent_statement'             : '0',
                'entity_descriptor_attributes' : templates.entity_descriptor_attribute % {
                    'name'     : 'wrappedValue',
                    'optional' : 'false',
                    'type'     : mapping.make_argument_type(mapping.schema.types[name].type)
                }
            })
            
        emitted_entities = set()
        entities_to_emit = mapping.schema.entities.keys()
        while len(emitted_entities) < len(mapping.schema.entities):
            for name, type in mapping.schema.entities.items():
                if name in emitted_entities: continue
                if len(type.supertypes) == 0 or set(type.supertypes) < emitted_entities:
                    constructor_arguments = mapping.get_assignable_arguments(type, include_derived = True)
                    entity_descriptor_attributes = []
                    for arg in constructor_arguments:
                        if not arg['is_inherited']:
                            tmpl = templates.entity_descriptor_attribute_enum if arg['argument_type_enum'] == 'IfcUtil::Argument_ENUMERATION' else templates.entity_descriptor_attribute
                            entity_descriptor_attributes.append(tmpl % {
                                'name'      : arg['name'],
                                'optional'  : 'true' if arg['is_optional'] else 'false',
                                'type'      : arg['argument_type_enum'],
                                'enum_type' : arg['argument_type']
                            })
                        
                    emitted_entities.add(name)
                    parent_statement = '0' if len(type.supertypes) != 1 else templates.entity_descriptor_parent % {
                        'type' : type.supertypes[0]
                    }
                    entity_descriptors.append(templates.entity_descriptor % {
                        'type'                         : name,
                        'parent_statement'             : parent_statement,
                        'entity_descriptor_attributes' : '\n'.join(entity_descriptor_attributes)
                    })
                    
        for name, enum in mapping.schema.enumerations.items():
            enumeration_descriptor_values = '\n'.join([templates.enumeration_descriptor_value % {
                'name' : v
            } for v in enum.values])
            enumeration_descriptors.append(templates.enumeration_descriptor % {
                'type'                          : name,
                'enumeration_descriptor_values' : enumeration_descriptor_values
            })
            
        for name, type in mapping.schema.entities.items():
            constructor_arguments = mapping.get_assignable_arguments(type, include_derived = True)
            statements = ''.join(templates.derived_field_statement_attrs % (a['index']-1) for a in constructor_arguments if a['is_derived'])
            if len(statements):
                derived_field_statements.append(templates.derived_field_statement % {
                    'type'       : name,
                    'statements' : statements
                })

        self.str = templates.rt_implementation % {
            'schema_name_upper'        : mapping.schema.name.upper(),
            'schema_name'              : mapping.schema.name.capitalize(),
            'entity_descriptors'       : '\n'.join(entity_descriptors),
            'enumeration_descriptors'  : '\n'.join(enumeration_descriptors),
            'derived_field_statements' : '\n'.join(derived_field_statements)
        }

        self.schema_name = mapping.schema.name.capitalize()
    def __repr__(self):
        return self.str
    def emit(self):
        f = open('%s-rt.cpp'%self.schema_name, 'w', encoding='utf-8')
        f.write(str(self))
        f.close()

