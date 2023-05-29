#ifndef __JSONIO_H
#define __JSONIO_H

#include <string>
#include <vector>
#include <list>
#include <map>
#include "strview.h"

template<class T> struct xio;


template<class T>
struct x2s_by_str
{
  bool x2s_found;
  strview_t x2s_str;
  T& x2s_result;

  x2s_by_str( strview_t _str, T* _out ) : x2s_found(false), x2s_str(_str), x2s_result(*_out) {}
  operator bool () const { return x2s_found; }
  template<size_t N>
  bool operator() ( const T&& v, const char (&s)[N] )
  {
    if( !x2s_found && x2s_str.equal(s) ) {
      x2s_found = true;
      x2s_result = v;
      return true;
    }
    return false;
  }
};

template<class T, class Out>
struct x2s_by_val
{
  bool x2s_found;
  const T& x2s_val;
  Out& x2s_result;

  x2s_by_val( const T& _val, Out* _out ) : x2s_found(false), x2s_val(_val), x2s_result(*_out) {}
  operator bool () const { return x2s_found; }
  template<size_t N>
  bool operator() ( const T& v, const char (&s)[N] )
  {
    if( !x2s_found && x2s_val == v ) {
      x2s_found = true;
      x2s_result = s;
      return true;
    }
    return false;
  }
};

struct x2s_dummy
{
  operator bool () const { return false; }
  bool operator() ( ... ) { return false; }
};

template<class T>
static bool x2s_value_( strview_t _str, T* _out )
{
  x2s_by_str<T> s( _str, _out );
  xio<T>::x2s_map_( s );
  return s;
}

template<class T>
static T x2s_value_( strview_t _str )
{
  T res; x2s_value_( _str, &res );
  return res;
}

template<class T, class Out>
static bool x2s_name_( T _val, Out* _out )
{
  x2s_by_val<T, Out> s( _val, _out );
  xio<T>::x2s_map_( s );
  return s;
}

template<class T>
static strview_t x2s_name_( T _val )
{
  strview_t res; x2s_name_( _val, &res );
  return res;
}


static inline uint8_t hexch_to_int_( const char ch )
{
  if( '0' <= ch && ch <='9' )
    return (uint8_t)(ch - '0');
  if( 'A' <= ch && ch <='F' )
    return (uint8_t)(ch - 'A' + 10);
  if( 'a' <= ch && ch <='f' )
    return (uint8_t)(ch - 'a' + 10);
  throw "Invalid hex char";
}

static void json_str_to_bin_( void* _out, const char* _data, size_t _size )
{
  uint8_t* v = (uint8_t*)_out;
  const char* p = _data;
  const char* e = p + _size;
  for( ; p < e; p += 2 ) {
    uint8_t b1 = hexch_to_int_( *p );
    uint8_t b2 = hexch_to_int_( *(p+1) );
    *v = (char)(b1 << 4 | b2);
    v++;
  }
}

static void json_str_to_bin_( std::string* _out, const char* _data, size_t _size )
{
  if( 0 != _size % 2 ) {
    throw "Invalid hex data length";
  }
  size_t n = _out->size();
  _out->resize( n + _size / 2 );
  char* v = (char*)_out->data() + n;
  json_str_to_bin_( v, _data, _size );
}

template< class S >
static void json_bin_to_str_( S& _out, const void* _data, size_t _size )
{
  _out.reserve( _out.size() + _size * 2 );
  size_t i = 0;
  const uint8_t* p = (const uint8_t*)_data;
  for( ; i < _size; ++i, ++p ) {
    _out += "0123456789ABCDEF"[((*p) >> 4)];
    _out += "0123456789ABCDEF"[((*p) & 0x0f)];
  }
}


struct JsonIn;
struct JsonInValue;
struct JsonOut;
struct JsonOutValue;

struct JsonInBinS
{
  std::string& v;
  JsonInBinS(std::string& _v) : v(_v) {}
};


struct JsonInBinX
{
  void*  v_data;
  size_t v_size;
  template< class T >
  JsonInBinX( T& _v ) : v_data(&_v), v_size(sizeof(_v)) {}
};

struct JsonOutBinX
{
  const void* v_data;
  size_t v_size;
  JsonOutBinX(const std::string& _v) : v_data(_v.data()), v_size(_v.size()) {}
  JsonOutBinX(const strview_t& _v) : v_data(_v.data()), v_size(_v.size()) {}
  template< class T >
  JsonOutBinX(const T& _v) : v_data(&_v), v_size(sizeof(_v)) {}
  const void* data() const { return v_data; }
       size_t size() const { return v_size; }
};

template<class F, class S>
struct XioFunc
{
  typedef S io_stream;
  typedef F io_type;
  //F io_func;
  //XioFunc( F _f ) : io_func(_f) {}
};





static inline bool is_json_ws_( const char c ) { return ' ' == c || '\t' == c || '\n' == c || '\r' == c; }
static inline bool is_json_qs_( const char c ) { return '"' == c || '\'' == c; }
static inline bool is_json_val_end_( const char c ) { return is_json_ws_(c) || ',' == c || '}' == c || ']' == c; }

static inline void json_trim_ws_( strview_t& x )
{
  for( ; !x.empty() && is_json_ws_(x.front()); x.pop_front() ) {}
  for( ; !x.empty() && is_json_ws_(x.back()); x.pop_back() ) {}
}

static inline bool json_trim_ch_( strview_t& x, const char c1, const char c2 )
{
  if( x.size() < 2 ) {
    throw std::string("expected ") + c1 + "..." + c2;
    //return false;
  }
  if( c1 != x.front() || c2 != x.back() ) {
    throw std::string("expected ") + c1 + "..." + c2;
    //return false;
  }
  x.pop_front();
  x.pop_back();
  return true;
}

static inline bool is_json_begin_( strview_t x )
{
  if( x.empty() )
    return true;
  const char* rI = x.end() - 1;
  const char* rE = x.begin() - 1;
  for( ; rI != rE && is_json_ws_(*rI); --rI) {}
  if( rI == rE )
    return true;
  if( '{' == *rI || '[' == *rI )
    return true;
  return false;
}

static inline size_t json_find_closing_quote_( const strview_t& x, size_t i, const char q )
{
  ASSERT( i > 0 );
  size_t n = x.size();
  for( ; i < n; ++i )
  {
    if( '\\' == x[i] ) {
      ++i;
      continue;
    }
    if( q == x[i] )
    {
      return i;
    }
  }
  throw std::string("expected closing quote ") + q;
  //return n;
}

static inline strview_t json_trim_quotes_( const strview_t& x )
{
  strview_t xx = x;
  if( xx.size() >= 2 && is_json_qs_(xx.front()) ) {
    ASSERT( x.front() == x.back() );
    xx.pop_front(); xx.pop_back();
  }
  return xx;
}

static inline bool json_skip_quotes_( strview_t& x, strview_t* v )
{
  ASSERT( x.size() >= 2 );
  char q = x[0];
  if( !is_json_qs_(q) )
    return false;
  size_t i = json_find_closing_quote_( x, 1, q );
  *v = x.trim_head( i + 1 );
  return true;
}

static inline bool json_skip_comma_( strview_t& x )
{
  for( ; !x.empty(); x.pop_front() )
  {
    const char ch = x.front();
    if( is_json_ws_(ch) )
      continue;
    if( ',' != ch )
      throw std::string("expected comma, but found: ") + ch;
    x.pop_front();
    return true;
  }
  return false;
}

static inline bool json_skip_( strview_t& x, strview_t* v, const char c1, const char c2 )
{
  ASSERT( x.size() >= 2 );
  if( c1 != x[0] )
    return false;
  size_t nest = 0;
  size_t i = 1, n = x.size();
  for( ; i < n;  ++i )
  {
    char ch = x[i];
    if( c2 == ch )
    {
      if( nest ) {
        nest--;
        continue;
      }
      *v = x.trim_head(i + 1);
      return true;
    }
    if( c1 == ch ) {
      nest++;
      continue;
    }
    if( is_json_qs_(ch) ) {
      i = json_find_closing_quote_( x, i + 1, ch );
      continue;
    }
  }
  throw std::string("expected closing ") + c2;
  //return false;
}

static inline strview_t json_pop_value_( strview_t& x )
{
  json_trim_ws_(x);
  if( x.size() >= 2 )
  {
    strview_t v;
    if( json_skip_quotes_(x, &v) )
      return v;
    if( json_skip_(x, &v, '{', '}') )
      return v;
    if( json_skip_(x, &v, '[', ']') )
      return v;
  }
  size_t i = 0;
  for( ; i < x.size() && !is_json_val_end_(x[i]); ++i ) {}
  return x.trim_head(i);
}

static inline void json_pop_param_value_( strview_t& x, strview_t* _p, strview_t* _v )
{
  strview_t p = x.trim_before( ':' );
  if( p.empty() ) {
    throw std::string("expected ':'");
  }
  json_trim_ws_(p);
  if( p.size() >= 2 && is_json_qs_(p.front()) ) {
    if( p.front() != p.back() ) {
      throw std::string("param name malformed");
    }
    p.pop_front(); p.pop_back();
  }
  if( p.empty() ) {
    throw std::string("param name empty");
  }
  *_p = p;
  x.pop_front(); // remove ':'
  *_v = json_pop_value_( x );
}

template< class T >
static inline strview_t json_enum_params_( strview_t& s, T _func )
{
  strview_t p, v;
  for( ; !s.empty(); )
  {
    json_pop_param_value_( s, &p, &v );
    json_trim_ws_( s );
    if( ',' == s[0] ) {
      s.pop_front();
      json_trim_ws_( s );
    }
    if( _func( p, v ) )
      return v;
  }
  return strview_t();
}

/////////////////////////////////////////////////////////// JsonIn /////////////////////////////////////////////////////////////

// used to workaround incomplete type in gcc/clang
template< class I, class T > struct use_incomplete { typedef I type; };

template< class T >
static inline bool json_read_( const strview_t& x, T& _v, decltype( &T::template serialize<JsonIn,T> ) _dummy )
{
  typename use_incomplete<JsonIn, T>::type  ji(x);
  T::serialize( ji, _v );
  return true;
}
template< class T >
static inline bool json_read_( const strview_t& x, T& _v, decltype( &xio<T>::template serialize<JsonIn,T> ) _dummy )
{
  typename use_incomplete<JsonIn, T>::type  ji(x);
  xio<T>::serialize( ji, _v );
  return true;
}
template< class T >
static inline bool json_read_( const strview_t& x, T& _v, decltype( &xio<T>::template x2s_map_<x2s_dummy> ) _dummy )
{
  strview_t xx = json_trim_quotes_(x);
  return x2s_value_( xx, &_v );
}
template< class T >
static inline bool json_read_( const strview_t& x, T& _v, ... )
{
  strview_t xx = json_trim_quotes_(x);
  return xio<T>::Read( xx, _v );
}
template< class X, class T >
static inline bool json_read_x_( const strview_t& x, T& _v )
{
  strview_t xx = json_trim_quotes_(x);
  return X::Read( xx, _v );
}
template< class T >
static inline bool json_read_list_( const strview_t& x, T& _v )
{
  strview_t xx = x;
  json_trim_ch_( xx, '[', ']' );
  json_trim_ws_( xx );
  _v.clear();
  while( !xx.empty() ) {
    strview_t xv = json_pop_value_( xx );
    typename T::value_type v;
    json_read_( xv, v, 0 );
    _v.push_back( std::move(v) );
    json_skip_comma_( xx );
    json_trim_ws_( xx );
  }
  return true;
}
template< class T >
static inline bool json_read_( const strview_t& x, std::list<T>& _v, int _dummy )
{
  return json_read_list_( x, _v );
}
template< class T >
static inline bool json_read_( const strview_t& x, std::vector<T>& _v, int _dummy )
{
  return json_read_list_( x, _v );
}

static inline bool json_read_string_( const strview_t& x, std::string& _v )
{
  strview_t xx = x;
  strview_t b, a;
  while( xx.split_by('\\', &b, &a) && !a.empty() ) {
    _v.append( b.data(), b.size() );
    switch( a.front() ) {
    case 'n': _v += '\n'; break;
    case 't': _v += '\t'; break;
    case '"': _v += '\"'; break;
    case '\'': _v += '\''; break;
    case '\\': _v += '\\'; break;
    default: throw std::string("unknown escape char: ") + a.front();
    }
    a.pop_front();
    xx = a;
  }
  _v.append( xx.data(), xx.size() );
  return true;
}

struct JsonInBin;
struct JsonInFlags;
struct JsonInBitFields;

struct JsonInValue
{
  strview_t x;
  JsonInValue( const strview_t& _x ) : x(_x) {}
  const char* data() const { return x.data(); }
  size_t      size() const { return x.size(); }
  bool        isnull() const { return x.isnull(); }
  bool        empty() const { return x.empty(); }

  template< class T > void operator() ( T& _v ) const
  {
    json_read_( x, _v, 0 );
    // TODO throw if ret false
  }

  template< class T > operator T () const
  {
    T v; (*this)(v); return v; // std::move(v);
  }

  template< class T, class F >
  void operator() ( T& _v, F _f ) const
  {
    typename F::io_stream ji(x);
    F::io_type::template serialize(ji, _v);
  }

  strview_t get() const
  {
    return json_trim_quotes_(x);
  }
  operator strview_t () const
  {
    return get();
  }
  operator std::string () const
  {
    strview_t v = get();
    return std::string(v.data(), v.size());
  }

  JsonInValue get( const strview_t& _n ) const
  {
    strview_t xx = x;
    json_trim_ch_( xx, '{', '}' );
    json_trim_ws_( xx );
    strview_t r = json_enum_params_( xx, [&_n] (const strview_t& p, const strview_t& v)
    {
      return p.equal(_n);
    } );
    // TODO throw if r.isnull()
    return JsonInValue(r);
  }
};

struct JsonInArray
{
  strview_t xx;
  JsonInArray(const strview_t& _x, bool _trim = true) : xx(_x) {
    if( _trim ) {
      json_trim_ch_(xx, '[', ']');
      json_trim_ws_(xx);
    }
  }
  JsonInArray(const JsonInValue& _x, bool _trim = true) : JsonInArray(_x.x, _trim) {}
  ~JsonInArray() {}
  bool empty() { return xx.empty(); }
  JsonInValue next()
  {
    ASSERT(!empty());
    strview_t xv = json_pop_value_(xx);
    json_skip_comma_(xx);
    json_trim_ws_(xx);
    return JsonInValue(xv);
  }
  template< class T > bool operator() (T& _v)
  {
    if (empty())
      return false;
    next()(_v);
    return true;
  }
  template< class T, class F >
  bool operator() (T& _v, F _f)
  {
    if (empty())
      return false;
    typename F::io_stream ji(next().x);
    F::io_type::template serialize(ji, _v);
    return true;
  }
};


struct JsonIn
{
  strview_t x;
  typedef std::map<strview_t, strview_t> params_t;
  params_t params_by_name;

  static JsonInBinS Bin(std::string& _v) { return JsonInBinS(_v); }
  template< class T >
  static JsonInBinX Bin(T& _v) { return JsonInBinX(_v); }

  static XioFunc<JsonInBin, strview_t> Bin() { return XioFunc<JsonInBin, strview_t>(); }

  template< class F >
  static XioFunc<F, JsonInFlags> Flags(F _f) { return XioFunc<F, JsonInFlags>(); }
  static XioFunc<void, JsonInFlags> Flags() { return XioFunc<void, JsonInFlags>(); }
  template< class F >
  static XioFunc<F, JsonInBitFields> BitFields(F _f) { return XioFunc<F, JsonInBitFields>(); }
  static XioFunc<void, JsonInBitFields> BitFields() { return XioFunc<void, JsonInBitFields>(); }

  template< typename T >
  JsonIn( const T& _x ) : x(_x)
  {
    json_trim_ws_( x );
    if( x.empty() )
      return;
    json_trim_ch_( x, '{', '}' );
    json_trim_ws_( x );
  }
  JsonIn( const JsonInValue& _x ) : x(_x)
  {
    if( x.empty() ) // value was not found
      return;
    json_trim_ch_( x, '{', '}' );
    json_trim_ws_( x );
  }

  template< class T > bool operator() ( T& _v ) const
  {
    T::template serialize( *this, _v ); // return xio<T>::Read( *this, _v );
    return true;
  }

  template< class T > void operator() ( const char* _n, T& _v )
  {
    (this->get(_n))( _v );
  }

  void operator() ( const char* _n, JsonInBinS _v )
  {
    (this->get(_n))( _v );
  }
  void operator() ( const char* _n, JsonInBinX _v )
  {
    (this->get(_n))( _v );
  }
  template< class T, class S, class F >
  void operator() ( const char* _n, T& _v, XioFunc<F, S> _f )
  {
    (this->get(_n))( _v, _f );
  }
  template< class T, class S >
  void operator() ( const char* _n, T& _v, XioFunc<void, S> _f )
  {
    (this->get(_n))( _v, XioFunc<T, S>() );
  }
  template< class T, class X >
  void operator() ( const char* _n, T& _v, xio<X> _f )
  {
    json_read_x_< xio<X> >( this->get(_n).x, _v );
  }

  void parse()
  {
    // modify x inside
    json_enum_params_( x, [this] (const strview_t& p, const strview_t& v)
    {
      params_by_name[p] = v; return false;
    } );
    ASSERT( x.empty() );
  }

  JsonInValue get( const strview_t& _n )
  {
    params_t::const_iterator pI = params_by_name.find( _n );
    if( pI != params_by_name.end() )
      return JsonInValue(pI->second);
    // modify x inside
    strview_t r = json_enum_params_( x, [this, &_n] (const strview_t& p, const strview_t& v)
    {
      params_by_name[p] = v; return p.equal(_n);
    } );
    // TODO throw if r.isnull()
    return JsonInValue(r);
  }

  template< size_t N >
  JsonInValue operator() ( const char (&_n)[N] )
  {
    return get( _n );
  }

  explicit operator bool() const { return true; }
};



struct JsonInBin
{
  template< typename S >
  static void serialize( S& s, std::string& p )
  {
    strview_t x = json_trim_quotes_(s);
    json_str_to_bin_(&p, x.data(), x.size());
  }
};

struct JsonInFlags
{
  strview_t x;
  JsonInFlags( const strview_t& _x ) : x(json_trim_quotes_(_x)) {}

  template< size_t N >
  bool find_flag( const char (&_n)[N] )
  {
    strview_t xx = x;
    strview_t vx;
    bool v = false;
    while( !v && vpnfc::strlist_pop_front(xx, &vx, ' ') ) {
      v = vx.equal(_n);
    }
    if( v && vx.begin() == x.begin() ) {
      // TODO assume there is no duplicates in x
      x = xx;
    }
    return v;
  }

  template<class T, size_t N>
  void operator () ( const char (&_n)[N], T& _v, T _bit )
  {
    bool v = find_flag( _n );
    if( v ) _v |= _bit; // set bit
    else _v &= ~_bit; // clear bit
  }

  template< class T, class Fi, size_t N >
  void operator() ( const char (&_n)[N], const T& _v, Fi _fin )
  {
    bool v = find_flag( _n );
    _fin(v);
  }
};

struct JsonInBitFields
{
  JsonIn x;
  JsonInBitFields( const strview_t& _x ) : x(_x) {}

  template<class T>
  void operator () ( const char* _n, T& _v, T _bit )
  {
    unsigned v;
    (x.get(_n))( v );
    if( v ) _v |= _bit; // set bit
    else _v &= ~_bit; // clear bit
  }

  template< class T, class Fi > 
  void operator() ( const char* _n, const T& _v, Fi _fin )
  {
    unsigned v;
    (x.get(_n))( v );
    _fin(v);
  }
};


/////////////////////////////////////////////////////////// JsonOut /////////////////////////////////////////////////////////////
struct json_out_t
{
  std::string& x;
  size_t indent;
  json_out_t(std::string& _x) : x(_x), indent(0) {}
  json_out_t(const json_out_t& _x) : x(_x.x), indent(_x.indent + 1) {}

  const char* data() const { return x.data(); }
  size_t size() const { return x.size(); }
  const char& back() const { return x.back(); }
  char& back() { return x.back(); }
  void reserve( size_t n ) { return x.reserve(n); }
  template <size_t N>
  void operator += ( const char (&_s)[N] ) { x += _s; }
  void operator += ( const char *_s ) { x += _s; }
  void operator += ( char _s ) { x += _s; }
  void append( const char *_data, size_t _size ) { x.append(_data, _size); }
};

static inline std::string& jostr( json_out_t& _x ) { return _x.x; }
static inline std::string& jostr( std::string& _x ) { return _x; }
static inline void joindent( json_out_t& _x ) { _x.x.append( _x.indent, '\t'); }
static inline void joindent( std::string& _x ) {}
static inline void joindent_end_scope( json_out_t& _x ) { _x.x += '\n'; if( _x.indent > 1 ) _x.x.append( _x.indent - 1, '\t'); }

// decltype( &T::template serialize<JsonOut,T> )
// decltype( T::serialize(x, _v) )

template< class S, class T >
static inline void json_write_( S& x, const T& _v, decltype( &T::template serialize<JsonOut,T> ) _dummy )
{
  typename use_incomplete<JsonOut, T>::type jo(x);
  T::serialize( jo, _v );
}
template< class S, class T >
static inline void json_write_( S& x, T& _v, decltype( &T::template serialize<JsonOut,T> ) _dummy )
{
  typename use_incomplete<JsonOut, T>::type jo(x);
  T::serialize( jo, _v );
}
template< class S, class T >
static inline void json_write_( S& x, const T& _v, decltype( &xio<T>::template serialize<JsonOut,T> ) _dummy )
{
  typename use_incomplete<JsonOut, T>::type jo(x);
  xio<T>::serialize( jo, _v );
}
template< class S, class T >
static inline void json_write_( S& x, const T& _v, decltype( &xio<T>::template x2s_map_<x2s_dummy> ) _dummy )
{
  x += "\"";
  strview_t xx;
  if( !x2s_name_( _v, &xx ) ) {
    throw "Unknown enum"; // xio<unsigned>::Write( x, (unsigned)_v ); // gcc error: incomplete type �xio<unsigned int>� used in nested name specifier
  }
  x.append( xx.data(), xx.size() );
  x += "\"";
}
template< class S, class T >
static inline typename xio<T>::is_numeric_type json_write_( S& x, const T& _v, int _dummy )
{
  xio<T>::Write( x, _v );
}
template< class S, class T >
static inline void json_write_( S& x, const T& _v, ...)
{
  x += "\"";
  xio<T>::Write( jostr(x), _v );
  x += "\"";
}
template< class X, class S, class T >
static inline typename X::is_numeric_type json_write_x_( S& x, const T& _v, int _dummy )
{
  X::Write( jostr(x), _v );
}
template< class X, class S, class T >
static inline void json_write_x_( S& x, const T& _v, ... )
{
  x += "\"";
  X::Write( jostr(x), _v );
  x += "\"";
}
template< class S, class T >
static inline void json_write_array_( S& x, const T& _v )
{
  x += "[";
  bool first = true;
  for( const auto& v : _v ) {
    if( first ) { first = false; } else { x += ", "; }
    json_write_( x, v, 0 );
  }
  x += "]";
}
template< class X, class S, class T >
static inline void json_write_array_x_( S& x, const T& _v )
{
  x += "[";
  bool first = true;
  for( const auto& v : _v ) {
    if( first ) { first = false; } else { x += ", "; }
    json_write_x_<X>( x, v, 0 );
  }
  x += "]";
}
template< class S, class T >
static inline void json_write_( S& x, const std::list<T>& _v, int _dummy )
{
  return json_write_array_( x, _v );
}
template< class S, class T >
static inline void json_write_( S& x, const std::vector<T>& _v, int _dummy )
{
  return json_write_array_( x, _v );
}
template< class X, class S, class T >
static inline void json_write_x_( S& x, const std::list<T>& _v, int _dummy )
{
  return json_write_array_x_<X>( x, _v );
}
template< class X, class S, class T >
static inline void json_write_x_( S& x, const std::vector<T>& _v, int _dummy )
{
  return json_write_array_x_<X>( x, _v );
}
template< class S >
static void json_write_string_( S& _out, const strview_t& _v )
{
  const char* to[] = { "\\\\", "\\\"", "\\n", "\\t" };
  vpnfc::strview_replace_chars( _v, jostr(_out), "\\\"\n\t", to );
}



struct JsonOutBin;
struct JsonOutFlags;
struct JsonOutBitFields;

struct JsonOutValue
{
  json_out_t& x;
  //JsonOutValue( std::string& _x ) : x(_x) {}
  JsonOutValue( json_out_t& _x ) : x(_x) {}

  template< class T > void operator() ( const T& _v )
  {
    json_write_(x, _v, 0);
  }
  template< class T > void operator() ( T& _v )
  {
    json_write_(x, _v, 0);
  }

  template< class T, class F > void operator() ( T& _v, F _f )
  {
    typename F::io_stream jo(x);
    F::io_type::template serialize(jo, _v);
  }

  template< class T > void operator = ( const T& _v )
  {
    (*this)(_v);
  }
};

struct JsonOutArray
{
  json_out_t x;
  bool first;
  JsonOutArray( std::string& _x ) : x(_x), first(true) { x += "["; }
  JsonOutArray( const json_out_t& _x ) : x(_x), first(true) { x += "["; }
  JsonOutArray( const JsonOutValue& _x ) : JsonOutArray(_x.x) {}
  ~JsonOutArray() { x += "]"; }
  template< class T > void operator() ( const T& _v )
  {
    if( first ) { first = false; } else { x += ", "; }
    json_write_(x, _v, 0);
  }
  JsonOutValue next()
  {
    if (first) { first = false; } else { x += ", "; }
    return JsonOutValue(x);
  }
};

struct JsonOut
{
  json_out_t x;
  JsonOut( std::string& _x ) : x(_x) { x += "{\n"; }
  JsonOut( const json_out_t& _x ) : x(_x) { x += "{\n"; }
  JsonOut( const JsonOutValue& _x ) : JsonOut(_x.x) {}
  ~JsonOut() { joindent_end_scope( x ); x += "}"; }

  template< class T >
  static JsonOutBinX Bin(const T& _v) { return JsonOutBinX(_v); }
  static XioFunc<JsonOutBin, json_out_t&> Bin() { return XioFunc<JsonOutBin, json_out_t&>(); }
  template< class F >
  static XioFunc<F, JsonOutFlags> Flags(F _f) { return XioFunc<F, JsonOutFlags>(); }
  static XioFunc<void, JsonOutFlags> Flags() { return XioFunc<void, JsonOutFlags>(); }
  template< class F >
  static XioFunc<F, JsonOutBitFields> BitFields(F _f) { return XioFunc<F, JsonOutBitFields>(); }
  static XioFunc<void, JsonOutBitFields> BitFields() { return XioFunc<void, JsonOutBitFields>(); }

  template< class T > void operator() ( const T& _v )
  {
    //x += "{\n";
    T::template serialize( *this, _v ); // xio<T>::Write( *this, _v );
    //x += "}\n";
  }

  template< class T > void operator() ( const char* _n, const T& _v )
  {
    (*this)( _n )( _v );
  }
  template< class T > void operator() ( const char* _n, T& _v )
  {
    (*this)( _n )( _v );
  }
  template< class T, class S, class F > void operator() ( const char* _n, T& _v, XioFunc<F, S> _f )
  {
    (*this)( _n )( _v, _f );
  }
  template< class T, class S > void operator() ( const char* _n, T& _v, XioFunc<void, S> _f )
  {
    (*this)( _n )( _v, XioFunc<T, S>() );
  }
  template< class T, class X > void operator() ( const char* _n, const T& _v, xio<X> _f )
  {
    (*this)( _n );
    json_write_x_< xio<X> >( x, _v, 0 );
  }

  template< size_t N >
  JsonOutValue operator() ( const char* (& _n)[N] )
  {
    if( !is_json_begin_(x) ) {
      x += ",\n";
    }
    joindent( x );
    x += "\"";
    for( const char* n: _n ) { x += n; }
    x += "\": ";
    return JsonOutValue(x);
  }
  JsonOutValue operator() ( const char* _n )
  {
    const char* n[] = {_n};
    return (*this)( n );
  }
  JsonOutArray array( const char* _n )
  {
    (*this)( _n );
    return JsonOutArray(x);
  }
  explicit operator bool() const { return true; }
};

struct JsonOutBin
{
  template< typename S, typename T >
  static void serialize( S& s, T* p )
  {
    s += "\"";
    json_bin_to_str_( s, &p, sizeof(p) );
    s += "\"";
  }
  template< typename S, typename T >
  static void serialize( S& s, T& p )
  {
    s += "\"";
    json_bin_to_str_( s, p.data(), p.size() );
    s += "\"";
  }
};

struct JsonOutFlags
{
  json_out_t x;
  JsonOutFlags(std::string& _x) : x(_x) { x += '\"'; }
  JsonOutFlags(json_out_t& _x) : x(_x) { x += '\"'; }
  ~JsonOutFlags()
  {
    if( x.back() == ' ' ) x.back() = '\"';
    else x += '\"';
  }

  void write_flag( const char* _n, bool _v )
  {
    if( !_v )
      return;
    x += _n;
    x += ' ';
  }

  template<class T>
  void operator () ( const char* _n, T _v, T _bit )
  {
    write_flag( _n, (_v & _bit) );
  }
  template< class T, class Fi > 
  void operator() ( const char* _n, T _v, Fi _fin )
  {
    write_flag( _n, !!_v );
  }
};

struct JsonOutBitFields
{
  JsonOut x;
  JsonOutBitFields(std::string& _x) : x(_x) {}
  JsonOutBitFields(json_out_t& _x) : x(_x) {}

  template<class T>
  void operator () ( const char* _s, T _v, T _bit )
  {
    x(_s)( (unsigned)!!(_v & _bit) );
  }
  template< class T, class Fi > 
  void operator() ( const char* _n, T _v, Fi _fin )
  {
    x( _n )( _v );
  }
};

/////////////////////////////////////////////////////////// xio /////////////////////////////////////////////////////////////

// default serialization -> call target specific static member - serialize()
template<class T> struct xio;
/*{
  typedef void is_composite_type;
  template< class R > static bool Read( R& _in, T& _v )
  {
    T::template serialize( _in, _v ); // return _in( _v );
    return true;
  }
  template< class W > static void Write( W& _out, const T& _v )
  {
    T::template serialize( _out, _v ); // _out( _v );
  }
};*/

template<> struct xio< std::string >
{
  template< class R > static bool Read( R& _in, std::string& _v )
  {
    _v.clear();
    return json_read_string_( _in, _v );
  }
  template< class W > static void Write( W& _out, const std::string& _v )
  {
    json_write_string_( _out, _v );
  }
};

template<> struct xio< strview_t >
{
  template< class R > static bool Read( R& _in, std::string& _v )
  {
    _v.clear();
    return json_read_string_( _in, _v );
  }
  template< class W > static void Write( W& _out, const strview_t& _v )
  {
    json_write_string_( _out, _v );
  }
};

template<> struct xio< const char* >
{
    template< class W > static void Write( W& _out, const char* _v )
    {
      json_write_string_( _out, _v );
    }
};

template<size_t N> struct xio< char [N] >
{
  template< class R > static bool Read( R& _in, char (&_v)[N] )
  {
    std::string buf;
    json_read_string_(_in, buf);
    if (N <= buf.size())
        return false;
    memcpy(_v, buf.data(), buf.size());
    buf[buf.size()] = 0;
    return true;
  }
  template< class W > static void Write( W& _out, const char (&_v)[N] )
  {
    json_write_string_(_out, _v);
  }
};

template<> struct xio< uint64_t >
{
  typedef void is_numeric_type;
  template< class R > static bool Read( R& _in, uint64_t& _v )
  {
    uint64_t r = 0;
    for( char c: _in ) {
      if( !('0' <= c && c <= '9') )
        return false;
      r = r * 10 + (c - '0'); // TODO check overflow
    }
    _v = r;
    return true;
  }
  template< class W > static void Write( W& _out, uint64_t _v )
  {
    char buf[64];
    SNPRINTF(buf, sizeof(buf), "%llu", (unsigned long long)_v);
    _out += buf;
  }
};

template<> struct xio< unsigned >
{
  typedef void is_numeric_type;
  template< class R > static bool Read( R& _in, unsigned& _v )
  {
    uint64_t x;
    if( !xio<uint64_t>::Read( _in, x ) )
      return false;
    _v = (unsigned)x;
    return true;
  }
  template< class W > static void Write( W& _out, unsigned _v )
  {
    return xio<uint64_t>::Write( _out, (uint64_t)_v );
  }
};

template<> struct xio< uint16_t >
{
  typedef void is_numeric_type;
  template< class R > static bool Read(R& _in, uint16_t& _v)
  {
    uint64_t x;
    if (!xio<uint64_t>::Read(_in, x))
      return false;
    _v = (uint16_t)x;
    return true;
  }
  template< class W > static void Write(W& _out, uint16_t _v)
  {
    return xio<uint64_t>::Write(_out, (uint64_t)_v);
  }
};

template<> struct xio<int64_t>
{
  typedef void is_numeric_type;
  template< class R > static bool Read(R& _in, int64_t& _v)
  {
    uint64_t r = 0;
    bool first = true, neg = false;
    for (char c : _in) {
      if (first && c == '-')
        neg = true;
      else {
        if (!('0' <= c && c <= '9'))
          return false;
        r = r * 10 + (c - '0'); // TODO check overflow
      }
      first = false;
    }
    _v = neg ? r * -1 : r;
    return true;
  }
  template< class W > static void Write(W& _out, int64_t _v)
  {
    char buf[64];
    SNPRINTF(buf, sizeof(buf), "%lld", (long long)_v);
    _out += buf;
  }
};

template<> struct xio<int32_t>
{
  typedef void is_numeric_type;
  template< class R > static bool Read(R& _in, int32_t& _v)
  {
    int64_t x;
    if (!xio<int64_t>::Read(_in, x))
      return false;
    _v = (int32_t)x;
    return true;
  }
  template< class W > static void Write(W& _out, int32_t _v)
  {
    return xio<int64_t>::Write(_out, (int64_t)_v);
  }
};

template<> struct xio< bool >
{
  typedef void is_numeric_type;
  template< class R > static bool Read( R& _in, bool& _v )
  {
    if( _in.equal("true") ) {
      _v = true;
      return true;
    }
    if( _in.equal("false") ) {
      _v = false;
      return true;
    }
    uint64_t x;
    if( !xio<uint64_t>::Read( _in, x ) )
      return false;
    _v = !!x;
    return true;
  }
  template< class W > static void Write( W& _out, bool _v )
  {
    if( _v )
      _out += "true";
    else
      _out += "false";
  }
};


template<> struct xio< JsonInBinS >
{
  template< class R > static bool Read( R& _in, JsonInBinS& _v )
  {
    json_str_to_bin_( &_v.v, _in.data(), _in.size() );
    return true;
  }
};

template<> struct xio< JsonInBinX >
{
  template< class R > static bool Read( R& _in, JsonInBinX& _v )
  {
    if( _in.size() != _v.v_size * 2 ) {
      return false; //throw "Invalid bin data length";
    }
    json_str_to_bin_( _v.v_data, _in.data(), _in.size() );
    return true;
  }
};

template<> struct xio< JsonOutBinX >
{
  template< class W > static void Write( W& _out, const JsonOutBinX& _v )
  {
    json_bin_to_str_( _out, _v.data(), _v.size() );
  }
};


#endif // #ifndef __JSONIO_H
