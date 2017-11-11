// -*- C++ -*-
#ifndef _SQULL_ORM_H
#define _SQULL_ORM_H

#include <sqlite3.h>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace squll {
  namespace utility {
// maps languge types to SQLite types
template <typename T> struct type_map {
  std::string type() const { return std::string(); }
};

template <> struct type_map<int> {
  std::string type() const { return "INT"; }
};

template <> struct type_map<unsigned> : public type_map<int> {};

template <> struct type_map<std::string> {
  std::string type() const { return "TEXT"; }
};

template <std::size_t N> struct tuple_size_t {};

template <typename T, typename F>
void tuple_apply(T _tuple, F _callable) {
  tuple_apply(_tuple, _callable,
	      tuple_size_t<std::tuple_size<T>::value>());
}

template <typename T, typename F>
void tuple_apply(const T , F , tuple_size_t<0>) { }

template <typename T, typename F, std::size_t N>
void tuple_apply(T &&_tuple, F &&_callable, tuple_size_t<N>) {
  tuple_apply(std::forward<T>(_tuple), std::forward<F>(_callable),
	      tuple_size_t<N - 1>());
  _callable(std::get<N - 1>(std::forward<T>(_tuple)));
}
}

namespace constraints {
struct autoincrement {
  std::string descr() const { return "AUTOINCREMENT"; }
};
  struct not_null {
    std::string descr() const { return "NOT NULL"; }
  };
}

namespace impl {
// RAII sqlite3_stmt guard - finalizes passed statement when goes out of scope
class sql_finalize_guard {
public:
  sql_finalize_guard(sqlite3_stmt *&pSt) : m_stat{pSt} {}
  ~sql_finalize_guard() {
    if (m_stat != nullptr) {
      sqlite3_finalize(m_stat);
    }
  }

private:
  sqlite3_stmt *&m_stat;
};

template <typename... Columns>
class table_impl_t {
public:
  table_impl_t(Columns... _cols)
    : m_columns(std::make_tuple(_cols...)) {}

  std::string generate_sql_description() const {
    std::string descr;
    constexpr std::size_t nColumns = std::tuple_size<columns_tuple_t>::value;
    std::size_t index = 0;
    utility::tuple_apply(m_columns, [&index, &descr, nColumns](auto& _c) {
	descr.append(_c.sql_description());
	if (index < nColumns - 1)
	  descr.append(",");
	++index;
      });
    return descr;
  }

private:
  typedef std::tuple<Columns...> columns_tuple_t;

  columns_tuple_t m_columns;
};

template <typename... Tables> class schema_impl_t {
public:
  schema_impl_t(sqlite3 *&_ph, Tables... _tables)
    : m_pHandler{_ph}, m_tables(std::make_tuple(_tables...)) {}

  void create_tables() {
    std::vector<std::string> tableQueries;
    tableQueries.reserve(std::tuple_size<tables_tuple_t>::value);
    
    utility::tuple_apply(m_tables, [&tableQueries](auto &_t) {
	std::string query = "CREATE TABLE IF NOT EXISTS " + _t.name() + "(" +
                         _t.generate_sql_description() + ");";
	tableQueries.emplace_back(query);
      });
    
    for (const std::string &query : tableQueries) {
      sqlite3_stmt *pSt{nullptr};
      sql_finalize_guard guard(pSt);

      int err_code = sqlite3_prepare_v2(m_pHandler, query.c_str(),
					query.length(), &pSt, nullptr);
      if (err_code != SQLITE_OK)
	throw std::runtime_error(std::string(sqlite3_errstr(err_code)));

      if (sqlite3_step(pSt) != SQLITE_OK)
	throw std::runtime_error(std::string(sqlite3_errstr(err_code)));
    }
  }

private:
  typedef std::tuple<Tables...> tables_tuple_t;

  sqlite3 *&m_pHandler;
  tables_tuple_t m_tables;
};
}


template <typename MappedType, typename MemberType, typename... Modifiers>
class column_t {
public:
  column_t(const std::string &_name, MemberType MappedType::*_pf,
           Modifiers... _mods)
      : m_name{_name}, m_pointer{_pf},
        m_constraints(std::make_tuple(_mods...)) {}

  std::string sql_description() const {
    std::string result =  m_name + " " + utility::type_map<field_t>().type();
    if (std::tuple_size<constraints_t>::value)
      result += " ";
    result += get_constraints_str();
    return result;
  }

private:
  typedef MappedType object_t;
  typedef MemberType field_t;
  typedef field_t object_t::*member_pointer_t;
  typedef std::tuple<Modifiers...> constraints_t;

  std::string m_name;
  member_pointer_t m_pointer{nullptr};
  constraints_t m_constraints;

  std::string get_constraints_str() const {
    std::string res;
    constexpr std::size_t constr_count = std::tuple_size<constraints_t>::value;
    std::size_t constr_index = 0;
    utility::tuple_apply(m_constraints, [&res, &constr_index, constr_count](auto& _c) {
	res.append(_c.descr());
	if (constr_index < constr_count - 1)
	  res.append(" ");
	constr_index++;
      });
    return res;
  }
};

template <typename... Tables> class schema_t {
public:
  schema_t(const std::string &_fn, Tables... _tabs)
      : m_impl{m_pHandler, _tabs...} {
    open(_fn);
    m_impl.create_tables();
  }

  ~schema_t() {
    if (m_pHandler != nullptr) {
      sqlite3_close(m_pHandler);
    }
  }

private:
  impl::schema_impl_t<Tables...> m_impl;
  sqlite3 *m_pHandler{nullptr};

  void open(const std::string &_fn) {
    int err_code =
        sqlite3_open_v2(_fn.c_str(), &m_pHandler,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

    if (SQLITE_OK != err_code)
      throw std::runtime_error(std::string(sqlite3_errstr(err_code)));
  }
};

template <typename... Columns> class table_t {
public:
  table_t(const std::string &_name, Columns... _cols)
      : m_name{_name}, m_impl{_cols...} {}
  const std::string &name() const { return m_name; }
  std::string generate_sql_description() const {
    return m_impl.generate_sql_description();
  }

private:
  std::string m_name;
  impl::table_impl_t<Columns...> m_impl;
};

template <typename... Tables>
schema_t<Tables...> schema(const std::string &_fn, Tables... _tabs) {
  return {_fn, _tabs...};
}

template <typename... Columns>
table_t<Columns...> table(const std::string &_name, Columns... _cols) {
  return {_name, _cols...};
}

template <typename MappedType, typename MemberType, typename... Modifiers>
column_t<MappedType, MemberType, Modifiers...>
column(const std::string &_name, MemberType MappedType::*_pm,
       Modifiers... _mods) {
  return {_name, _pm, _mods...};
}
}

#endif
