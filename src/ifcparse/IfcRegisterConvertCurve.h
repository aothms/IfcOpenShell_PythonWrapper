#include "IfcRegisterUndef.h"
#define CURVE(T) \
	if ( l->is(T::Class()) ) return convert(reinterpret_pointer_cast<IfcBaseClass,T>(l),r);
#include "IfcRegisterDef.h"

#include "IfcRegister.h"