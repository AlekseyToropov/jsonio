#ifndef __STRVIEW_H
#define __STRVIEW_H


struct isch_t
{
  const char ch; isch_t( const char _ch ) : ch(_ch) {}
  bool operator () ( const char _ch ) { return ch == _ch; }
};

struct strview_t
{
  const char* sv_data;
  const char* sv_endp;

  strview_t() : sv_data(nullptr), sv_endp(nullptr) {}
  strview_t( const char* _s, size_t _l ) : sv_data(_s), sv_endp(_s + _l) {}
  strview_t( char* _s, size_t _l ) : sv_data(_s), sv_endp(_s + _l) {}
  strview_t( const char* _s ) : sv_data(_s), sv_endp(_s + strlen(_s)) {}
  strview_t( char* _s ) : sv_data(_s), sv_endp(_s + strlen(_s)) {}
  //strview_t( const char* _b, const char* _e ) : sv_data(_b), sv_endp(_e) {}
  template< size_t N >
  strview_t( const char (&_s)[N] ) : sv_data(_s), sv_endp(_s + N - 1) {}
  template< typename T >
  strview_t( const T& _s ) : sv_data(_s.data()), sv_endp(_s.data() + _s.size()) {}

  void assign( const char* _s, size_t _l ) { sv_data = _s; sv_endp = _s + _l; }
  void clear() { sv_endp = sv_data; }

  const char* data() const { return sv_data; }
  size_t      size() const { return sv_endp - sv_data; }
  const char* begin() const { return sv_data; }
  const char* end() const   { return sv_endp; }

  bool isnull() const { return nullptr == sv_data; }
  bool empty() const  { return 0 == size(); }
  bool equal( const char* _s ) const { return size() == strlen(_s) && 0 == MEMCMP(_s , data(), size()); }
  bool equal( const char* _s, size_t _len ) const { return size() == _len && (_s == data() || 0 == MEMCMP(data(), _s, _len)); }
  template< size_t N >
  bool equal( const char (&_s)[N] ) const { return equal(_s, N - 1); }
  template< class T >
  bool equal( const T& _s ) const { return equal(_s.data(), _s.size()); }

  bool head_equal( const strview_t& _head ) const { return size() >= _head.size() && 0 == MEMCMP(data(), _head.data(), _head.size()); }

  bool less( const char* _s, size_t _len ) const
  {
    if( size() < _len )
      return true;
    if( size() == _len && MEMCMP(data(), _s, _len) < 0 )
      return true;
    return false;
  }
  template< class T >
  bool less( const T& _s ) const { return less(_s.data(), _s.size()); }
  template< class T >
  bool operator < ( const T& _s ) const { return less(_s.data(), _s.size()); }

  char operator [] (size_t i) const { ASSERT( i <= size() ); return sv_data[i]; }
  char front() const { ASSERT( !empty() ); return *sv_data; }
  char back() const  { ASSERT( !empty() ); return *(sv_endp - 1); }
  void pop_front()   { ASSERT( !empty() ); sv_data++; }
  void pop_back()    { ASSERT( !empty() ); sv_endp--; }

  strview_t trim_head( size_t pos ) { ASSERT( pos <= size() ); strview_t r(sv_data, pos); sv_data += pos; return r; }
  strview_t trim_tail( size_t pos ) { ASSERT( pos <= size() ); sv_endp -= pos; return strview_t(sv_endp, pos); }
  void  remove_prefix( size_t N ) { ASSERT( N <= size() ); sv_data += N; };
  void  remove_suffix( size_t N ) { ASSERT( N <= size() ); sv_endp -= N; };

  template< class F >
  bool split_by( F _delim, strview_t* before, strview_t* after ) const
  {
    const char* p = sv_data;
    for( ; p < sv_endp && !_delim(*p); ++p ) {}
    if( p == sv_endp )
      return false;
    if( before ) {
      before->sv_data = sv_data;
      before->sv_endp = p;
    }
    if( after ) {
      after->sv_data = p + 1;
      after->sv_endp = sv_endp;
    }
    return true;
  }
  bool split_by( const char c, strview_t* before, strview_t* after ) const { return split_by( isch_t(c), before, after ); }

  template< class F >
  strview_t trim_before( F _delim )
  {
    strview_t r;
    if( !split_by( _delim, &r, nullptr ) )
      return strview_t();
    this->sv_data = r.sv_endp;
    return r;
  }
  strview_t trim_before( const char c ) { return trim_before( isch_t(c) ); }

  template< class F >
  strview_t trim_after( F _delim )
  {
    strview_t r;
    if( !split_by( _delim, nullptr, &r ) )
      return strview_t();
    this->sv_endp = r.sv_data;
    return r;
  }
  strview_t trim_after( const char c ) { return trim_after( isch_t(c) ); }
};

struct strview_c_str_t
{
  strview_t& sv;
  char       zero_ch;
  strview_c_str_t( strview_t& _s ) : sv(_s) { zero_ch = *_s.end(); (*(char*)sv.end()) = '\0'; }
  ~strview_c_str_t() { (*(char*)sv.end()) = zero_ch; }
  const char* c_str() const { return sv.data(); }
  const char* data() const { return sv.data(); }
  size_t      size() const { return sv.size(); }
};

namespace vpnfc
{

static inline bool is_ws_( const char c ) { return (' ' == c || '\t' == c || '\n' == c || '\r' == c); }
static inline bool is_comma_( const char ch ) { return (',' == ch || ';' == ch); }

template< class T, class F >
static inline void trim_head_( T& x, F _func )
{
  for( ; !x.empty() && _func(x.front()); x.pop_front() ) {}
}
template< class T, class F >
static inline void trim_tail_( T& x, F _func )
{
  for( ; !x.empty() && _func(x.back()); x.pop_back() ) {}
}
template< class T, class F >
static inline void trim_ends_( T& x, F _func )
{
  trim_head_( x, _func );
  trim_tail_( x, _func );
}
template< class T >
static inline void trim_ws( T& x )
{
  return trim_ends_( x, is_ws_ );
}

template< class T >
static inline bool trim_ends_( T& x, const char c1, const char c2 )
{
  if( x.size() < 2 || c1 != x.front() || c2 != x.back() ) {
    return false;
  }
  x.pop_front();
  x.pop_back();
  return true;
}

template< class T, class O, class F >
static inline bool strlist_pop_front( T& x, O* _out, F _delim )
{
  trim_head_( x, is_ws_ );
  if( x.empty() ) {
    return false;
  }
  T r;
  if( !x.split_by( _delim, &r, &x ) ) {
    r = x; ASSERT( !r.empty() );
    x.clear();
  }
  trim_tail_( r, is_ws_ ); // r may be empty
  _out->assign( r.data(), r.size() );
  return true;
}

template< class T, class O >
static inline bool strlist_pop_front( T& x, O* _out )
{
  return strlist_pop_front( x, _out, is_comma_ );
}

template< class T, class F >
static inline bool contain( const T& x, F _check )
{
  for( char c: x ) {
    if( _check(c) )
      return true;
  }
  return false;
}
template< class T >
static inline bool contain( const T& x, char _ch )
{
  return contain( x, isch_t(_ch) );
}


template< size_t N >
static strview_t strview_replace_chars( const strview_t& s, std::string& _out, const char (&_from)[N], const char* (&_to)[N-1] )
{
  size_t posI = 0;
  size_t posE = posI;
  for( ; posE < s.size(); ++posE )
  {
    size_t i = 0;
    for( ; i < N - 1 && _from[i] != s[posE]; ++i ) {}
    if( i == N - 1 )
      continue;
    ASSERT( _from[i] == s[posE] );
    _out.append( s.data() + posI, posE - posI );
    _out.append( _to[i] );
    posI = posE + 1;
  }
//  if( 0 == posI )
//    return s;
  ASSERT( posE == s.size() ); ASSERT( posI <= s.size() );
  _out.append( s.data() + posI, posE - posI );
  return _out;
}

} // namespace vpnfc

#if 0
class TLogMessage;

namespace vpnfc
{
template< class T, class P1, class P2 >
static inline T& TLogMessage_addStr_gcc_wrap_(T& t, P1 p1, P2 p2) { return t.addStr(p1, p2); };
template< class T >
static inline TLogMessage& out_str_( TLogMessage& out, const T& str )
{
  return TLogMessage_addStr_gcc_wrap_( out, str.data(), str.size() );
}
template< class T, class S >
static inline T& out_str_( T& out, const S& str )
{
  out << str;
  return out;
}
} // namespace vpnfc

template< class T >
static inline T& operator << ( T& out, const strview_t& str )
{
  return vpnfc::out_str_( out, str );
}
#endif

#endif // #ifndef __STRVIEW_H
