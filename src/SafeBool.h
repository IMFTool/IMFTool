/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Safe Bool implementation by Krzysztof Czainski
 */
#pragma once

//! Safe Bool: https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Safe_bool
class AbstractSaveBool {

public:
	typedef void (AbstractSaveBool::*bool_type)() const;
	void this_type_does_not_support_comparisons() const {}

protected:
	AbstractSaveBool() {}
	AbstractSaveBool(const AbstractSaveBool&) {}
	AbstractSaveBool& operator=(const AbstractSaveBool&) { return *this; }
	~AbstractSaveBool() {}
};


//! Safe Bool for testability without virtual function: https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Safe_bool 
template <typename T = void>
class SafeBool : private AbstractSaveBool {

public:
	operator bool_type() const {
		return (static_cast<const T*>(this))->BooleanTest()
			? &AbstractSaveBool::this_type_does_not_support_comparisons : 0;
	}

protected:
	~SafeBool() {}
};


//! Safe Bool for testability with virtual function: https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Safe_bool 
template<>
class SafeBool<void> : private AbstractSaveBool{

public:
	operator bool_type() const {
		return BooleanTest()
			? &AbstractSaveBool::this_type_does_not_support_comparisons : 0;
	}

protected:
	virtual bool BooleanTest() const = 0;
	virtual ~SafeBool() {}
};

template <typename T>
bool operator==(const SafeBool<T>& lhs, bool b) {
	return b == static_cast<bool>(lhs);
}

template <typename T>
bool operator==(bool b, const SafeBool<T>& rhs) {
	return b == static_cast<bool>(rhs);
}

template <typename T, typename U>
bool operator==(const SafeBool<T>& lhs, const SafeBool<U>& rhs) {
	lhs.this_type_does_not_support_comparisons();
	return false;
}

template <typename T, typename U>
bool operator!=(const SafeBool<T>& lhs, const SafeBool<U>& rhs) {
	lhs.this_type_does_not_support_comparisons();
	return false;
}
