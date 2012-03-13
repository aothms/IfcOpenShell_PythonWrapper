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

# <pep8 compliant>

###############################################################################
#                                                                             #
# Based on the Wavefront OBJ File importer by Campbell Barton                 #
#                                                                             #
###############################################################################

bl_info = {
    "name": "IfcBlender",
    "description": "Import files in the "\
        "Industry Foundation Classes (.ifc) file format",
    "author": "Thomas Krijnen, IfcOpenShell",
    "blender": (2, 5, 8),
    "api": 37702,
    "location": "File > Import",
    "warning": "",
    "wiki_url": "http://sourceforge.net/apps/"\
        "mediawiki/ifcopenshell/index.php",
    "tracker_url": "http://sourceforge.net/tracker/?group_id=543113",
    "category": "Import-Export"}

import sys   
max_unicode = 0x110000-1 if sys.platform[0:5] == 'linux' else 0x10000-1
wrong_unicode = max_unicode != sys.maxunicode
if wrong_unicode:
    print("\nWarning: wrong unicode representation detected, switching to "\
        "compatibility layer for text transferral, may result in undefined "\
        "behaviour, please use offical release from http://blender.org\n")
    
if "bpy" in locals():
    import imp
    if "IfcImport" in locals():
        imp.reload(IfcImport)

import bpy
import mathutils
from bpy.props import StringProperty, IntProperty, BoolProperty
from bpy_extras.io_utils import ImportHelper

major,minor = bpy.app.version[0:2]
transpose_matrices = minor >= 62

bpy.types.Object.ifc_id = IntProperty(name="IFC Entity ID",
    description="The STEP entity instance name")
bpy.types.Object.ifc_guid = StringProperty(name="IFC Entity GUID",
    description="The IFC Globally Unique Identifier")
bpy.types.Object.ifc_name = StringProperty(name="IFC Entity Name",
    description="The optional name attribute")
bpy.types.Object.ifc_type = StringProperty(name="IFC Entity Type",
    description="The STEP Datatype keyword")


def import_ifc(filename, use_names, process_relations):
    global wrong_unicode
    from . import IfcImport
    print("Reading %s..."%bpy.path.basename(filename))
    if wrong_unicode:
        valid_file = IfcImport.InitUCS2( 
            ''.join(['\0']+['\0%s'%s for s in filename]+['\0\0'])
        )
    else:
        valid_file = IfcImport.Init(filename)
    if not valid_file:
        IfcImport.CleanUp()
        return False
    print("Done reading file")
    id_to_object = {}
    id_to_parent = {}
    id_to_matrix = {}
    old_progress = -1
    print("Creating geometry...")
    while True:
        ob = IfcImport.Get()
        
        if wrong_unicode:
            ob_name = ''.join([chr(c) for c in ob.name_as_intvector()])
            ob_type = ''.join([chr(c) for c in ob.type_as_intvector()])
            ob_guid = ''.join([chr(c) for c in ob.guid_as_intvector()])
        else:
            ob_name, ob_type, ob_guid = ob.name, ob.type, ob.guid
        
        f = ob.mesh.faces
        v = ob.mesh.verts
        m = ob.matrix
        t = ob_type[0:21]
        nm = ob_name if len(ob_name) and use_names else ob_guid

        verts = [[v[i], v[i + 1], v[i + 2]] \
            for i in range(0, len(v), 3)]
        faces = [[f[i], f[i + 1], f[i + 2]] \
            for i in range(0, len(f), 3)]

        me = bpy.data.meshes.new('mesh%d' % ob.mesh.id)
        me.from_pydata(verts, [], faces)
        if t in bpy.data.materials:
            mat = bpy.data.materials[t]
            mat.use_fake_user = True
        else:
            mat = bpy.data.materials.new(t)
        me.materials.append(mat)

        bob = bpy.data.objects.new(nm, me)
        mat = mathutils.Matrix(([m[0], m[1], m[2], 0],
            [m[3], m[4], m[5], 0],
            [m[6], m[7], m[8], 0],
            [m[9], m[10], m[11], 1]))
        if transpose_matrices: mat.transpose()
        
        if process_relations:
            id_to_matrix[ob.id] = mat
        else:
            bob.matrix_world = mat
        bpy.context.scene.objects.link(bob)

        bpy.context.scene.objects.active = bob
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.normals_make_consistent()
        bpy.ops.object.mode_set(mode='OBJECT')

        bob.ifc_id, bob.ifc_guid, bob.ifc_name, bob.ifc_type = \
            ob.id, ob_guid, ob_name, ob_type

        bob.hide = ob_type == 'IfcSpace' or ob_type == 'IfcOpeningElement'
        bob.hide_render = bob.hide
        
        if ob.id not in id_to_object: id_to_object[ob.id] = []
        id_to_object[ob.id].append(bob)

        if ob.parent_id > 0:
            id_to_parent[ob.id] = ob.parent_id
            
        progress = IfcImport.Progress() // 2
        if progress > old_progress:
            print("\r[" + "#" * progress + " " * (50 - progress) + "]", end="")
            old_progress = progress
        if not IfcImport.Next():
            break

    print("\rDone creating geometry" + " " * 30)

    id_to_parent_temp = dict(id_to_parent)
    
    if process_relations:
        print("Processing relations...")

    while len(id_to_parent_temp) and process_relations:
        id, parent_id = id_to_parent_temp.popitem()
        
        if parent_id in id_to_object:
            bob = id_to_object[parent_id][0]
        else:
            parent_ob = IfcImport.GetObject(parent_id)
            if parent_ob.id == -1:
                bob = None
            else:
                if wrong_unicode:
                    parent_ob_name = ''.join(
                        [chr(c) for c in parent_ob.name_as_intvector()])
                    parent_ob_type = ''.join(
                        [chr(c) for c in parent_ob.type_as_intvector()])
                    parent_ob_guid = ''.join(
                        [chr(c) for c in parent_ob.guid_as_intvector()])
                else:
                    parent_ob_name, parent_ob_type, parent_ob_guid = \
                        parent_ob.name, parent_ob.type, parent_ob.guid
                    
                m = parent_ob.matrix
                nm = parent_ob_name if len(parent_ob_name) and use_names \
                    else parent_ob_guid
                bob = bpy.data.objects.new(nm, None)
                
                mat = mathutils.Matrix((
                    [m[0], m[1], m[2], 0],
                    [m[3], m[4], m[5], 0],
                    [m[6], m[7], m[8], 0],
                    [m[9], m[10], m[11], 1]))
                if transpose_matrices: mat.transpose()
                id_to_matrix[parent_ob.id] = mat
                
                bpy.context.scene.objects.link(bob)

                bob.ifc_id = parent_ob.id
                bob.ifc_name, bob.ifc_type, bob.ifc_guid = \
                    parent_ob_name, parent_ob_type, parent_ob_guid

                if parent_ob.parent_id > 0:
                    id_to_parent[parent_id] = parent_ob.parent_id
                    id_to_parent_temp[parent_id] = parent_ob.parent_id
                if parent_id not in id_to_object: id_to_object[parent_id] = []
                id_to_object[parent_id].append(bob)
        if bob:
            for ob in id_to_object[id]:
                ob.parent = bob

    id_to_matrix_temp = dict(id_to_matrix)

    while len(id_to_matrix_temp):
        id, matrix = id_to_matrix_temp.popitem()
        parent_id = id_to_parent.get(id, None)
        parent_matrix = id_to_matrix.get(parent_id, None)
        for ob in id_to_object[id]:
            if parent_matrix:
                ob.matrix_local = parent_matrix.inverted() * matrix
            else:
                ob.matrix_world = matrix
            
    if process_relations:
        print("Done processing relations")
    
    if not wrong_unicode:
        txt = bpy.data.texts.new("%s.log"%bpy.path.basename(filename))
        txt.from_string(IfcImport.GetLog())

    IfcImport.CleanUp()
    
    return True


class ImportIFC(bpy.types.Operator, ImportHelper):
    bl_idname = "import_scene.ifc"
    bl_label = "Import .ifc file"

    filename_ext = ".ifc"
    filter_glob = StringProperty(default="*.ifc", options={'HIDDEN'})

    use_names = BoolProperty(name="Use entity names",
        description="Use entity names rather than GlobalIds for objects",
        default=True)
    process_relations = BoolProperty(name="Process relations",
        description="Convert containment and aggregation" \
            " relations to parenting" \
            " (warning: may be slow on large files)",
        default=False)

    def execute(self, context):
        global wrong_unicode
        if wrong_unicode and sys.platform[0:5] != 'linux':
            self.report({'ERROR'},
                'Your version of Blender is incompatible with IfcBlender\n' \
                'Please use the offical release from http://blender.org instead'
            )
        elif not import_ifc(self.filepath, self.use_names, self.process_relations):
            self.report({'ERROR'},
                'Unable to parse .ifc file or no geometrical entities found'
            )
        return {'FINISHED'}


def menu_func_import(self, context):
    self.layout.operator(ImportIFC.bl_idname,
        text="Industry Foundation Classes (.ifc)")


def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()
