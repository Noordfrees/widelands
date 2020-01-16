/*
 * Copyright (C) 2002-2019 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef WL_EDITOR_SCRIPTING_BUILTIN_H
#define WL_EDITOR_SCRIPTING_BUILTIN_H

#include <memory>

#include <boost/function.hpp>

#include "editor/scripting/scripting.h"

class FunctionBase;

/************************************************************
                      Builtin functions
************************************************************/

// Wrapper for a (static) FunctionBase object, for use in kBuiltinFunctions
struct BuiltinFunctionInfo {
	BuiltinFunctionInfo(std::string u, boost::function<std::string()> d, FunctionBase* f)
	   : function(f), unique_name(u), description(d) {
	}
	~BuiltinFunctionInfo() {
	}

	const std::unique_ptr<FunctionBase> function;
	// internal name, unique among all kBuiltinFunctions entries
	const std::string unique_name;
	// Implemented as a function to make it translatable
	const boost::function<std::string()> description;

	DISALLOW_COPY_AND_ASSIGN(BuiltinFunctionInfo);
};

// All supported builtin functions.
const extern BuiltinFunctionInfo* kBuiltinFunctions[];
// Quick access to a builtin by its unique name
const BuiltinFunctionInfo& builtin_f(const std::string&);

/************************************************************
                       Property
************************************************************/

// Used to access a member variable of a variable of builtin type. Since only builtins
// have properties, you do not instantiate this class. Use kBuiltinProperties instead.
// Property needs to be embedded in FS_GetProperty or FS_SetProperty to be used for anything.
class Property {
public:
	Property(const std::string& n, bool ro, VariableType c, VariableType t)
	   : call_on_(c), type_(t), name_(n), read_only_(ro) {
		assert(type_ != VariableType::Nil);
	}

	~Property() {
	}

	VariableType get_type() const {
		return type_;
	}
	VariableType get_class() const {
		return call_on_;
	}
	const std::string& get_name() const {
		return name_;
	}
	bool read_only() const {
		return read_only_;
	}

private:
	VariableType call_on_;
	VariableType type_;
	std::string name_;
	bool read_only_;

	DISALLOW_COPY_AND_ASSIGN(Property);
};

/************************************************************
                      Builtin properties
************************************************************/

// Wrapper for a (static) Property object, for use in kBuiltinProperties
struct BuiltinPropertyInfo {
	BuiltinPropertyInfo(std::string u, boost::function<std::string()> d, Property* p)
	   : property(p), unique_name(u), description(d) {
	}
	~BuiltinPropertyInfo() {
	}

	const std::unique_ptr<Property> property;
	// internal name, unique among all kBuiltinProperties entries
	const std::string unique_name;
	// Implemented as a function to make it translatable
	const boost::function<std::string()> description;

	DISALLOW_COPY_AND_ASSIGN(BuiltinPropertyInfo);
};

// All supported builtin properties.
const extern BuiltinPropertyInfo* kBuiltinProperties[];
// Quick access to a builtin by its unique name
const BuiltinPropertyInfo& builtin_p(const std::string&);

#endif  // end of include guard: WL_EDITOR_SCRIPTING_BUILTIN_H
