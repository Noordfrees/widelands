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

#ifndef WL_EDITOR_SCRIPTING_OPERATORS_H
#define WL_EDITOR_SCRIPTING_OPERATORS_H

#include "editor/scripting/constexpr.h"

/************************************************************
              Mathematical operators: + - * /
************************************************************/

// Abstract base class for all operators with two arguments
class OperatorBase : public Assignable {
public:
	~OperatorBase() override {
	}
	const VariableType& type() const override {
		return output_type_;
	}

	Assignable* get_a() const {
		return a_;
	}
	void set_a(Assignable& a) {
		a_ = &a;
	}
	Assignable* get_b() const {
		return b_;
	}
	void set_b(Assignable& b) {
		b_ = &b;
	}

	std::set<uint32_t> references() const override;

	void load(FileRead&, Loader&) override;
	void load_pointers(const ScriptingLoader&, Loader&) override;
	void save(FileWrite&) const override;
	void write_lua(int32_t, FileWrite&) const override;

	std::string readable() const override;

	void selftest() const override;

	virtual std::string op() const = 0;

protected:
	OperatorBase(const VariableType& in, const VariableType& out, Assignable*, Assignable*);

private:
	Assignable* a_;
	Assignable* b_;
	VariableType input_type_;
	VariableType output_type_;

	DISALLOW_COPY_AND_ASSIGN(OperatorBase);
};

#define OPERATOR_FACTORY(name, i, o, sign)                                                         \
	class Operator##name : public OperatorBase {                                                    \
	public:                                                                                         \
		Operator##name(Assignable* a, Assignable* b)                                                 \
		   : OperatorBase(VariableType(VariableTypeID::i), VariableType(VariableTypeID::o), a, b) {  \
		}                                                                                            \
		ScriptingObject::ID id() const override {                                                    \
			return ScriptingObject::ID::Operator##name;                                               \
		}                                                                                            \
		inline std::string op() const override {                                                     \
			return sign;                                                                              \
		}                                                                                            \
		DISALLOW_COPY_AND_ASSIGN(Operator##name);                                                    \
	};

OPERATOR_FACTORY(Add, Integer, Integer, "+")
OPERATOR_FACTORY(Subtract, Integer, Integer, "-")
OPERATOR_FACTORY(Multiply, Integer, Integer, "*")
OPERATOR_FACTORY(Divide, Integer, Integer, "/")
OPERATOR_FACTORY(Modulo, Integer, Integer, "%")

OPERATOR_FACTORY(Less, Integer, Boolean, "<")
OPERATOR_FACTORY(LessEq, Integer, Boolean, "<=")
OPERATOR_FACTORY(Greater, Integer, Boolean, ">")
OPERATOR_FACTORY(GreaterEq, Integer, Boolean, ">=")

OPERATOR_FACTORY(MathematicalEquals, Integer, Boolean, "==")
OPERATOR_FACTORY(MathematicalUnequal, Integer, Boolean, "~=")

OPERATOR_FACTORY(LogicalEquals, Boolean, Boolean, "==")
OPERATOR_FACTORY(LogicalUnequal, Boolean, Boolean, "~=")

OPERATOR_FACTORY(And, Boolean, Boolean, "and")
OPERATOR_FACTORY(Or, Boolean, Boolean, "or")

#undef OPERATOR_FACTORY

// String concatenation with '..'
class OperatorStringConcat : public Assignable {
public:
	OperatorStringConcat(Assignable*, Assignable*);
	~OperatorStringConcat() override {
	}
	const VariableType& type() const override {
		return kVTID_String;
	}
	ScriptingObject::ID id() const override {
		return ScriptingObject::ID::OperatorStringConcat;
	}

	void set_a(Assignable&);
	void set_b(Assignable&);
	Assignable* get_a() const {
		return a_;
	}
	Assignable* get_b() const {
		return b_;
	}

	std::set<uint32_t> references() const override;

	void load(FileRead&, Loader&) override;
	void load_pointers(const ScriptingLoader&, Loader&) override;
	void save(FileWrite&) const override;
	void write_lua(int32_t, FileWrite&) const override;

	std::string readable() const override;

	void selftest() const override;

private:
	Assignable* a_;
	Assignable* b_;

	DISALLOW_COPY_AND_ASSIGN(OperatorStringConcat);
};

// Logical not
class OperatorNot : public Assignable {
public:
	OperatorNot(Assignable* a) : a_(a) {
	}
	~OperatorNot() override {
	}
	const VariableType& type() const override {
		return kVTID_Boolean;
	}

	Assignable* get_a() const {
		return a_;
	}
	void set_a(Assignable& a) {
		a_ = &a;
	}

	std::set<uint32_t> references() const override;

	void load(FileRead&, Loader&) override;
	void load_pointers(const ScriptingLoader&, Loader&) override;
	void save(FileWrite&) const override;
	void write_lua(int32_t, FileWrite&) const override;

	std::string readable() const override;
	ScriptingObject::ID id() const override {
		return ScriptingObject::ID::OperatorNot;
	}

	void selftest() const override;

private:
	Assignable* a_;

	DISALLOW_COPY_AND_ASSIGN(OperatorNot);
};

// Whether the parameter is nil. Replaces implicit conversions to boolean for type safety.
class OperatorIsNil : public Assignable {
public:
	OperatorIsNil(Assignable* a) : a_(a) {
	}
	~OperatorIsNil() override {
	}
	const VariableType& type() const override {
		return kVTID_Boolean;
	}

	Assignable* get_a() const {
		return a_;
	}
	void set_a(Assignable& a) {
		a_ = &a;
	}

	std::set<uint32_t> references() const override;

	void load(FileRead&, Loader&) override;
	void load_pointers(const ScriptingLoader&, Loader&) override;
	void save(FileWrite&) const override;
	void write_lua(int32_t, FileWrite&) const override;

	std::string readable() const override;
	ScriptingObject::ID id() const override {
		return ScriptingObject::ID::OperatorIsNil;
	}

	void selftest() const override;

private:
	Assignable* a_;

	DISALLOW_COPY_AND_ASSIGN(OperatorIsNil);
};

#endif  // end of include guard: WL_EDITOR_SCRIPTING_OPERATORS_H
