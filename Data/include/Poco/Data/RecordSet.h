//
// RecordSet.h
//
// $Id: //poco/Main/Data/include/Poco/Data/RecordSet.h#7 $
//
// Library: Data
// Package: DataCore
// Module:  RecordSet
//
// Definition of the RecordSet class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef Data_RecordSet_INCLUDED
#define Data_RecordSet_INCLUDED


#include "Poco/Data/Data.h"
#include "Poco/Data/Extraction.h"
#include "Poco/Data/Statement.h"
#include "Poco/Data/RowIterator.h"
#include "Poco/Data/BLOB.h"
#include "Poco/String.h"
#include "Poco/DynamicAny.h"
#include "Poco/Exception.h"


namespace Poco {
namespace Data {


class Session;


class Data_API RecordSet: private Statement
	/// RecordSet provides access to data returned from a query.
	/// Data access indices (row and column) are 0-based, as usual in C++.
	/// 
	/// Recordset provides navigation methods to iterate through the
	/// recordset, retrieval methods to extract data, and methods
	/// to get metadata (type, etc.) about columns.
	///
	/// To work with a RecordSet, first create a Statement, execute it, and
	/// create the RecordSet from the Statement, as follows:
	///
	///     Statement select(session);
	///     select << "SELECT * FROM Person";
	///     select.execute();
	///     RecordSet rs(select);
	///
	/// The number of rows in the RecordSet can be limited by specifying
	/// a limit for the Statement.
{
public:
	typedef std::map<std::size_t, Row*> RowMap;
	typedef RowIterator Iterator;

	using Statement::isNull;

	explicit RecordSet(const Statement& rStatement);
		/// Creates the RecordSet.

	explicit RecordSet(Session& rSession, const std::string& query);
		/// Creates the RecordSet.

	~RecordSet();
		/// Destroys the RecordSet.

	Statement& operator = (const Statement& stmt);
		/// Assignment operator.

	std::size_t rowCount() const;
		/// Returns the number of rows in the recordset.

	std::size_t columnCount() const;
		/// Returns the number of rows in the recordset.

	template <class T, class C>
	const Column<T,C>& column(const std::string& name) const
		/// Returns the reference to the first Column with the specified name.
	{
		return column<T,C>(columnPosition<T,C>(name));
	}

	template <class T, class C>
	const Column<T,C>& column(std::size_t pos) const
		/// Returns the reference to column at specified location.
	{
		typedef const InternalExtraction<T,C>* ExtractionVecPtr;

		const AbstractExtractionVec& rExtractions = extractions();

		std::size_t s = rExtractions.size();
		if (0 == s || pos >= s)
			throw RangeException(format("Invalid column index: %z", pos));
		
		ExtractionVecPtr pExtraction = dynamic_cast<ExtractionVecPtr>(rExtractions[pos].get());

		if (pExtraction)
		{
			return pExtraction->column();
		}
		else 
		{
			throw Poco::BadCastException(format("Type cast failed!\nColumn: %z\nTarget type:\t%s",  
				pos,
				std::string(typeid(T).name())));
		}
	}

	Row& row(std::size_t pos);
		/// Returns reference to row at position pos.
		/// Rows are lazy-created and cached.

	template <class T>
	const T& value(std::size_t col, std::size_t row) const
		/// Returns the reference to data value at [col, row] location.
	{
		switch (storage())
		{
		case STORAGE_VECTOR:
		case STORAGE_UNKNOWN:
			return column<T, std::vector<T> >(col).value(row);
		case STORAGE_LIST:
			return column<T, std::list<T> >(col).value(row);
		case STORAGE_DEQUE:
			return column<T, std::deque<T> >(col).value(row);
		default:
			throw IllegalStateException("Invalid storage setting.");
		}
	}

	template <class T>
	const T& value(const std::string& name, std::size_t row) const
		/// Returns the reference to data value at named column, row location.
	{
		switch (storage())
		{
		case STORAGE_VECTOR:
		case STORAGE_UNKNOWN:
			return column<T, std::vector<T> >(name).value(row);
		case STORAGE_LIST:
			return column<T, std::list<T> >(name).value(row);
		case STORAGE_DEQUE:
			return column<T, std::deque<T> >(name).value(row);
		default:
			throw IllegalStateException("Invalid storage setting.");
		}
	}

	DynamicAny value(std::size_t col, std::size_t row) const;
		/// Returns the data value at column, row location.

	DynamicAny value(const std::string& name, std::size_t row) const;
		/// Returns the data value at named column, row location.

	template <typename C>
	DynamicAny nvl(const std::string& name, const C& deflt)
		/// Returns the value in the named column of the current row
		/// if the value is not NULL, or deflt otherwise.
	{
		if (isNull(name))
			return DynamicAny(deflt);
		else
			return value(name);
	}

	template <typename C>
	DynamicAny nvl(std::size_t index, const C& deflt)
		/// Returns the value in the given column of the current row
		/// if the value is not NULL, or deflt otherwise.
	{
		if (isNull(index))
			return DynamicAny(deflt);
		else
			return value(index);
	}

	const RowIterator& begin();
		/// Moves the row cursor to the first row and returns the pointer to row.

	bool moveFirst();
		/// Moves the row cursor to the first row.
		///
		/// Returns true if there is at least one row in the RecordSet,
		/// false otherwise.

	bool moveNext();
		/// Moves the row cursor to the next row.
		///
		/// Returns true if the row is available, or false
		/// if the end of the record set has been reached and
		/// no more rows are available.

	bool movePrevious();
		/// Moves the row cursor to the previous row.
		///
		/// Returns true if the row is available, or false
		/// if there are no more rows available.

	const RowIterator& end();
		/// Moves the row cursor to the last row and returns null pointer.

	bool moveLast();
		/// Moves the row cursor to the last row.
		///
		/// Returns true if there is at least one row in the RecordSet,
		/// false otherwise.

	DynamicAny value(const std::string& name);
		/// Returns the value in the named column of the current row.

	DynamicAny value(std::size_t index);
		/// Returns the value in the given column of the current row.

	DynamicAny operator [] (const std::string& name);
		/// Returns the value in the named column of the current row.

	DynamicAny operator [] (std::size_t index);
		/// Returns the value in the named column of the current row.

	MetaColumn::ColumnDataType columnType(std::size_t pos) const;
		/// Returns the type for the column at specified position.

	MetaColumn::ColumnDataType columnType(const std::string& name) const;
		/// Returns the type for the column with specified name.

	const std::string& columnName(std::size_t pos) const;
		/// Returns column name for the column at specified position.

	std::size_t columnLength(std::size_t pos) const;
		/// Returns column maximum length for the column at specified position.

	std::size_t columnLength(const std::string& name) const;
		/// Returns column maximum length for the column with specified name.

	std::size_t columnPrecision(std::size_t pos) const;
		/// Returns column precision for the column at specified position.
		/// Valid for floating point fields only (zero for other data types).

	std::size_t columnPrecision(const std::string& name) const;
		/// Returns column precision for the column with specified name.
		/// Valid for floating point fields only (zero for other data types).

	bool isNull(const std::string& name);
		/// Returns true if column value of the current row is null.

private:
	RecordSet();

	template<class T, class C>
	std::size_t columnPosition(const std::string& name) const
		/// Returns the position of the column with specified name.
	{
		typedef const InternalExtraction<T,C>* ExtractionVecPtr;

		const AbstractExtractionVec& rExtractions = extractions();
		AbstractExtractionVec::const_iterator it = rExtractions.begin();
		AbstractExtractionVec::const_iterator end = rExtractions.end();
		
		for (; it != end; ++it)
		{
			ExtractionVecPtr pExtraction = dynamic_cast<ExtractionVecPtr>(it->get());

			if (pExtraction)
			{
				const Column<T,C>& col = pExtraction->column();
				if (0 == Poco::icompare(name, col.name()))
					return col.position();
			}
		}

		throw NotFoundException(format("Unknown column name: %s", name));
	}

	std::size_t    _currentRow;
	RowIterator*   _pBegin;
	RowIterator*   _pEnd;
	RowMap         _rowMap;
};


///
/// inlines
///
inline std::size_t RecordSet::rowCount() const
{
	poco_assert (extractions().size());
	return extractions()[0].get()->numOfRowsHandled();
}


inline std::size_t RecordSet::columnCount() const
{
	return extractions().size();
}


inline Statement& RecordSet::operator = (const Statement& stmt)
{
	_currentRow = 0;
	return Statement::operator = (stmt);
}


inline DynamicAny RecordSet::value(const std::string& name)
{
	return value(name, _currentRow);
}


inline DynamicAny RecordSet::value(std::size_t index)
{
	return value(index, _currentRow);
}


inline DynamicAny RecordSet::operator [] (const std::string& name)
{
	return value(name, _currentRow);
}


inline DynamicAny RecordSet::operator [] (std::size_t index)
{
	return value(index, _currentRow);
}


inline MetaColumn::ColumnDataType RecordSet::columnType(std::size_t pos)const
{
	return metaColumn(static_cast<UInt32>(pos)).type();
}


inline MetaColumn::ColumnDataType RecordSet::columnType(const std::string& name)const
{
	return metaColumn(name).type();
}


inline const std::string& RecordSet::columnName(std::size_t pos) const
{
	return metaColumn(static_cast<UInt32>(pos)).name();
}


inline std::size_t RecordSet::columnLength(std::size_t pos) const
{
	return metaColumn(static_cast<UInt32>(pos)).length();
}


inline std::size_t RecordSet::columnLength(const std::string& name)const
{
	return metaColumn(name).length();
}


inline std::size_t RecordSet::columnPrecision(std::size_t pos) const
{
	return metaColumn(static_cast<UInt32>(pos)).precision();
}


inline std::size_t RecordSet::columnPrecision(const std::string& name)const
{
	return metaColumn(name).precision();
}


inline bool RecordSet::isNull(const std::string& name)
{
	return isNull(metaColumn(name).position(), _currentRow);
}


inline const RowIterator& RecordSet::end()
{
	if (!_pEnd)
		_pEnd = new RowIterator(*this, true);

	return *_pEnd;
}


} } // namespace Poco::Data


#endif // Data_RecordSet_INCLUDED
