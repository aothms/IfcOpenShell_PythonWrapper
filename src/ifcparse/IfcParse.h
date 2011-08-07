/********************************************************************************
*                                                                              *
* This file is part of IfcOpenShell.                                           *
*                                                                              *
* IfcOpenShell is free software: you can redistribute it and/or modify         *
* it under the terms of the Lesser GNU General Public License as published by  *
* the Free Software Foundation, either version 3.0 of the License, or          *
* (at your option) any later version.                                          *
*                                                                              *
* IfcOpenShell is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
* Lesser GNU General Public License for more details.                          *
*                                                                              *
* You should have received a copy of the Lesser GNU General Public License     *
* along with this program. If not, see <http://www.gnu.org/licenses/>.         *
*                                                                              *
********************************************************************************/

/********************************************************************************
 *                                                                              *
 * This file provides functions for loading an IFC file into memory and access  *
 * its entities either by ID, by an Ifc2x3::Type or by reference                * 
 *                                                                              *
 ********************************************************************************/

#ifndef IFCPARSE_H
#define IFCPARSE_H

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstring>
#include <map>
#include <exception>

#include "../ifcparse/SharedPointer.h"
#include "../ifcparse/IfcUtil.h"
#include "../ifcparse/Ifc2x3.h"

const int BUF_SIZE = (32 * 1024 * 1024);

namespace IfcParse {

	class Entity;
	class Entities;
	typedef SHARED_PTR<Entity> EntityPtr;
	typedef SHARED_PTR<Entities> EntitiesPtr;

	//
	// Reads a file in chunks of BUF_SIZE and provides 
	// functions to randomly access its contents
	//
	class File {
	private:
		std::ifstream stream;
		char* buffer;
		unsigned int ptr;
		unsigned int len;
		unsigned int offset;
		void ReadBuffer(bool inc=true);
	public:
		bool valid;
		bool eof;
		unsigned int size;
		File(const std::string& fn);
		char Peek();
		char Read(unsigned int offset);
		void Inc();		
		void Close();
		void Seek(unsigned int offset);
		unsigned int Tell();
	};

	typedef unsigned int Token;

	//
	// Provides functions to convert Tokens to binary data
	// Tokens are merely offsets to where they can be read in the file
	//
	class TokenFunc {
	private:
		static bool startsWith(Token t, char c);
	public:
		static unsigned int Offset(Token t);
		static bool isString(Token t);
		static bool isIdentifier(Token t);
		static bool isOperator(Token t, char op = 0);
		static bool isEnumeration(Token t);
		static bool isDatatype(Token t);
		static int asInt(Token t);
		static bool asBool(Token t);
		static float asFloat(Token t);
		static std::string asString(Token t);
		static std::string toString(Token t);
	};

	//
	// Functions for creating Tokens from an arbitary file offset
	// The first 4 bits are reserved for Tokens of type ()=,;$*
	//
	Token TokenPtr(unsigned int offset);
	Token TokenPtr(char c);	
	Token TokenPtr();

	//
	// Interprets a file as a sequential stream of Tokens
	//
	class Tokens {
	private:
		File* file;
	public:
		Tokens(File* f);
		Token Next();
		std::string TokenString(unsigned int offset);
	};

	//
	// Argument of type list, e.g.
	// #1=IfcDirection((1.,0.,0.));
	//                 ==========
	//
	class ArgumentList: public Argument {
	private:
		std::vector<ArgumentPtr> list;
		void Push(ArgumentPtr l);
	public:
		ArgumentList(Tokens* t, std::vector<unsigned int>& ids);
		operator int() const;
		operator bool() const;
		operator float() const;
		operator std::string() const;
		operator std::vector<float>() const;
		operator std::vector<int>() const;
		operator std::vector<std::string>() const;
		operator IfcUtil::IfcSchemaEntity() const;
		operator SHARED_PTR<IfcUtil::IfcAbstractSelect>() const;
		operator IfcEntities() const;
		unsigned int Size() const;
		ArgumentPtr operator [] (unsigned int i) const;
		std::string toString() const;
		bool isNull() const;
	};

	//
	// Argument of type scalar or string, e.g.
	// #1=IfcVector(#2,1.0);
	//              == ===
	//
	class TokenArgument : public Argument {
	private:
		Token token;
	public: 
		TokenArgument(Token t);
		operator int() const;
		operator bool() const;
		operator float() const;
		operator std::string() const;
		operator std::vector<float>() const;
		operator std::vector<int>() const;
		operator std::vector<std::string>() const;
		operator IfcUtil::IfcSchemaEntity() const;
		operator SHARED_PTR<IfcUtil::IfcAbstractSelect>() const;
		operator IfcEntities() const;
		unsigned int Size() const;
		ArgumentPtr operator [] (unsigned int i) const;
		std::string toString() const;
		bool isNull() const;
	};

	//
	// Argument of an IFC type
	// #1=IfcTrimmedCurve(#2,(IFCPARAMETERVALUE(0.)),(IFCPARAMETERVALUE(1.)),.T.,.PARAMETER.);
	//                        =====================   =====================
	//
	class EntityArgument : public Argument {
	private:		
		IfcUtil::IfcSchemaEntity entity;		
	public:
		EntityArgument(IfcUtil::IfcSchemaEntity e);
		operator int() const;
		operator bool() const;
		operator float() const;
		operator std::string() const;
		operator std::vector<float>() const;
		operator std::vector<int>() const;
		operator std::vector<std::string>() const;
		operator IfcUtil::IfcSchemaEntity() const;
		operator SHARED_PTR<IfcUtil::IfcAbstractSelect>() const;
		operator IfcEntities() const;
		unsigned int Size() const;
		ArgumentPtr operator [] (unsigned int i) const;
		std::string toString() const;
		bool isNull() const;
	};

	//
	// Entity defined in an IFC file, e.g.
	// #1=IfcDirection((1.,0.,0.));
	// ============================
	//
	class Entity : public IfcAbstractEntity {
	private:
		ArgumentPtr args;
		Ifc2x3::Type::Enum _type;
	public:
		unsigned int _id;
		unsigned int offset;		
		Entity(unsigned int i, Tokens* t);
		Entity(unsigned int i, Tokens* t, unsigned int o);
		IfcEntities getInverse(Ifc2x3::Type::Enum c = Ifc2x3::Type::ALL);
		IfcEntities getInverse(Ifc2x3::Type::Enum c, int i, const std::string& a);
		void Load(std::vector<unsigned int>& ids, bool seek=false);
		ArgumentPtr getArgument (unsigned int i);
		std::string toString();
		std::string datatype();
		Ifc2x3::Type::Enum type();
		bool is(Ifc2x3::Type::Enum v);
		unsigned int id();
	};

	class IfcException : public std::exception {
	private:
		std::string error;
	public:
		IfcException(std::string e);
		~IfcException () throw ();
		const char* what() const throw();
	};
}

typedef IfcUtil::IfcSchemaEntity IfcEntity;
typedef IfcEntities IfcEntities;
typedef std::map<Ifc2x3::Type::Enum,IfcEntities> MapEntitiesByType;
typedef std::map<unsigned int,IfcEntity> MapEntityById;
typedef std::map<unsigned int,IfcEntities> MapEntitiesByRef;
typedef std::map<unsigned int,unsigned int> MapOffsetById;

//
// Several static convenience functions and variables
//
class Ifc {
private:
	static MapEntitiesByType bytype;
	static MapEntityById byid;
	static MapEntitiesByRef byref;
	static MapOffsetById offsets;
	static unsigned int lastId;
	static std::ostream* log1;
	static std::ostream* log2;
public:
	static void SetOutput(std::ostream* l1, std::ostream* l2);
	static void LogMessage(const std::string& type, const std::string& message, const SHARED_PTR<IfcAbstractEntity>& entity=SHARED_PTR<IfcAbstractEntity>());
	static IfcParse::File* file;
	static IfcParse::Tokens* tokens;
	template <class T>
	static typename T::list EntitiesByType() {
		IfcEntities e = EntitiesByType(T::Class());
		typename T::list l ( new IfcTemplatedEntityList<T>() ); \
		for ( IfcEntityList::it it = e->begin(); it != e->end(); ++ it ) { \
			l->push(reinterpret_pointer_cast<IfcUtil::IfcBaseClass,T>(*it));\
		}
		return l;
	}
	static IfcEntities EntitiesByType(Ifc2x3::Type::Enum t);
	static IfcEntities EntitiesByReference(int id);
	static IfcEntity EntityById(int id);
	static bool Init(const std::string& fn);
	static void Dispose();
	static float LengthUnit;
	static float PlaneAngleUnit;
	static int CircleSegments;
};

float UnitPrefixToValue( Ifc2x3::IfcSIPrefix::IfcSIPrefix v );

#endif
