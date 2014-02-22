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

import sys
import string
from pyparsing import *

class Expression:
    def __init__(self, contents):
        self.contents = contents[0]
    def __repr__(self):
        if self.op is None: return repr(self.contents)
        c = [isinstance(c,str) and c or str(c) for c in self.contents]
        if "%s" in self.op: return self.op % (" ".join(c))
        else: return "(%s)" % (" %s "%self.op).join(c)
    def __iter__(self):
        return self.contents.__iter__()

class Union(Expression):
    op = "|"

class Concat(Expression):
    op = "+"

class Optional(Expression):
    op = "Optional(%s)"

class Repeated(Expression):
    op = "ZeroOrMore(%s)"

class Term(Expression):
    op = None

class Keyword:
    def __init__(self, contents):
        self.contents = contents[0]
    def __repr__(self):
        return self.contents

class Terminal:
    def __init__(self, contents):
        self.contents = contents[0]
    def __repr__(self):
        s = self.contents
        is_keyword = len(s) >= 4 and s[0::len(s)-1] == '""' and \
            all(c in alphanums+"_" for c in s[1:-1])
        ty = "CaselessKeyword" if is_keyword else "CaselessLiteral"
        return "%s(%s)" % (ty, s)


LPAREN = Suppress("(")
RPAREN = Suppress(")")
LBRACK = Suppress("[")
RBRACK = Suppress("]")
LBRACE = Suppress("{")
RBRACE = Suppress("}")
EQUALS = Suppress("=")
VBAR   = Suppress("|")
PERIOD = Suppress(".")
HASH   = Suppress("#")

identifier = Word(alphanums+"_")
keyword    = Word(alphanums+"_").setParseAction(Keyword)
expression = Forward()
optional   = Group(LBRACK + expression + RBRACK).setParseAction(Optional)
repeated   = Group(LBRACE + expression + RBRACE).setParseAction(Repeated)
terminal   = quotedString.setParseAction(Terminal)
term       = (keyword | terminal | optional | repeated | (LPAREN + expression + RPAREN)).setParseAction(Term)
concat     = Group(term + OneOrMore(term)).setParseAction(Concat)
factor     = concat | term
union      = Group(factor + OneOrMore(VBAR + factor)).setParseAction(Union)
rule       = identifier + EQUALS + expression + PERIOD

expression << (union | factor)

grammar = OneOrMore(Group(rule))
grammar.ignore(HASH + restOfLine)

express = grammar.parseFile(sys.argv[1])

def find_keywords(expr, li = None):
    if li is None: li = []
    if isinstance(expr, Term):
        expr = expr.contents
    if isinstance(expr, Keyword):
        li.append(repr(expr))
        return li
    elif isinstance(expr, Expression):
        for term in expr:
            find_keywords(term, li)
    return set(li)

actions = {
    'type_decl'                 : "lambda t: TypeDeclaration(t)",
    'entity_decl'               : "lambda t: EntityDeclaration(t)",
    'underlying_type'           : "lambda t: UnderlyingType(t)",
    'enumeration_type'          : "lambda t: EnumerationType(t)",
    'aggregation_types'         : "lambda t: AggregationType(t)",
    'general_aggregation_types' : "lambda t: AggregationType(t)",
    'select_type'               : "lambda t: SelectType(t)",
    'binary_type'               : "lambda t: BinaryType(t)",
    'subtype_declaration'       : "lambda t: SubtypeExpression(t)",
    'derive_clause'             : "lambda t: AttributeList('derive', t)",
    'derived_attr'              : "lambda t: DerivedAttribute(t)",
    'inverse_clause'            : "lambda t: AttributeList('inverse', t)",
    'inverse_attr'              : "lambda t: InverseAttribute(t)",
    'bound_spec'                : "lambda t: BoundSpecification(t)",
    'explicit_attr'             : "lambda t: ExplicitAttribute(t)",
}

to_emit = set(id for id, expr in express)
emitted = set()
to_combine = set(["simple_id"])
to_ignore = set(["where_clause", "supertype_constraint", "unique_clause"])
statements = []

while True:
    emitted_in_loop = set()
    for id, expr in express:
        kws = find_keywords(expr)
        found = [k in emitted for k in kws]
        if id in to_emit and all(found):
            emitted_in_loop.add(id)
            emitted.add(id)
            stmt = "(%s)" % expr
            if id in to_combine:
                stmt = "originalTextFor(Combine%s)" % stmt
            if id in actions:
                stmt = "%s.setParseAction(%s)" % (stmt, actions[id])
            statements.append("%s = %s" % (id, stmt))
    to_emit -= emitted_in_loop
    if not emitted_in_loop: break

for id in to_emit:
    action = ".setParseAction(%s)" % actions[id] if id in actions else ""
    statements.append("%s = Forward()%s" % (id, action))

for id in to_emit:
    expr = [e for k, e in express if k == id][0]
    stmt = "(%s)" % expr
    if id in to_combine:
        stmt = "Suppress%s" % stmt
    statements.append("%s << %s" % (id, stmt))

print ("""import sys
from pyparsing import *
from nodes import *

%s

import schema
import mapping

import header
import enum_header
import implementation
import rt_header
import rt_implementation

syntax.ignore(Regex(r"\((?:\*(?:[^*]*\*+)+?\))"))
ast = syntax.parseFile(sys.argv[1])
schema = schema.Schema(ast)
mapping = mapping.Mapping(schema)

header.Header(mapping).emit()
enum_header.EnumHeader(mapping).emit()
implementation.Implementation(mapping).emit()
rt_header.RuntimeTypingHeader(mapping).emit()
rt_implementation.RuntimeTypingImplementation(mapping).emit()

def retry():
    import imp
    imp.reload(rt_header)
    imp.reload(rt_implementation)
    rt_header.RuntimeTypingHeader(mapping).emit()
    rt_implementation.RuntimeTypingImplementation(mapping).emit()
"""%('\n'.join(statements)))
