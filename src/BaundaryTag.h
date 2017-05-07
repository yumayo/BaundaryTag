#pragma once
#include <iostream>
#include <stdarg.h>
std::string format( char const * str, ... )
{
    static char buf[2048];
    va_list args;
    va_start( args, str );
    vsnprintf( buf, 2048, str, args );
    va_end( args );

    return buf;
}
void log( char const * str, ... )
{
    static char buf[2048];
    va_list args;
    va_start( args, str );
    vsnprintf( buf, 2048, str, args );
    va_end( args );

    std::cout << buf << std::endl;
}
void logMemory( char* memory, size_t memoryByte)
{
    std::string output = ">[ ";
    size_t i;
    // 一番最後のメモリだけ出力の形が違うので - 1 しています。
    for ( i = 0; i < memoryByte - 1; ++i )
    {
        if ( ( i % 16 ) == 15 )
        {
            output += format( "%02X", memory[i] & 0x000000FF );
            output += " ]\n>[ ";
        }
        else
        {
            if ( ( i % 4 ) == 3 )
            {
                output += format( "%02X | ", memory[i] & 0x000000FF );
            }
            else
            {
                output += format( "%02X", memory[i] & 0x000000FF );
            }
        }
    }
    output += format( "%02X", memory[i] & 0x000000FF );
    log( "%s ]", output.c_str( ) );
    log( "" );
}
using u_int = unsigned int;
#pragma pack(push, 1)
class BaundaryTag
{
public:
    struct State
    {
        static const char USE;
        static const char BEGIN;
        static const char END;
    };
public:
    BaundaryTag( u_int useByte, char state )
        : m_state( state )
        , m_useByte( useByte )
    {
        setupOverallByte( );
    }
    BaundaryTag( u_int useByte )
        : BaundaryTag( useByte, 0 )
    {
    }
    BaundaryTag* create( u_int useByte )
    {
        for ( BaundaryTag* itr = this;
              !itr->IsEnd( );
              itr = reinterpret_cast<BaundaryTag*>( itr->getNextHeaderPointer( ) ) )
        {
            // 空部屋じゃないなら切ります。
            if ( itr->IsUse( ) ) continue;
            // ヘッダーを加味した容量を確保できるか確認して、無理なら切ります。
            if ( itr->getUseByte( ) < HEADERBYTE + useByte + FOOTERBYTE ) continue;
            // ここまでくるものは確保できる容量があり、空部屋のものだけです。

            auto itrHaveUseByte = itr->getUseByte( );

            auto created = new( itr ) BaundaryTag( useByte, State::USE | ( itr->m_state & State::BEGIN ) );

            u_int splitUseByte = itrHaveUseByte - ( HEADERBYTE + useByte + FOOTERBYTE );
            auto splited = new( created->getNextHeaderPointer( ) ) BaundaryTag( splitUseByte );

            log( "created[%d] splited[%d]", useByte, splitUseByte );

            return created;
        }
        return nullptr;
    }
    char* getHeaderPointer( )
    {
        return reinterpret_cast<char*>( this );
    }
    char* getDataPointer( )
    {
        return getHeaderPointer( ) + HEADERBYTE;
    }
    char* getFooterPointer( )
    {
        return getHeaderPointer( ) + HEADERBYTE + m_useByte;
    }
    char* getNextHeaderPointer( )
    {
        return getFooterPointer( ) + FOOTERBYTE;
    }
    char* getPrevHeaderPointer( )
    {
        u_int prevOverallByte = *reinterpret_cast<u_int*>( getHeaderPointer( ) - FOOTERBYTE );
        return getHeaderPointer( ) - prevOverallByte;
    }
    u_int getOverallByte( )
    {
        u_int* footer = reinterpret_cast<u_int*>( getFooterPointer( ) );
        u_int overallByte = *footer;
        return overallByte;
    }
    void setupOverallByte( )
    {
        u_int* footer = reinterpret_cast<u_int*>( getFooterPointer( ) );
        u_int overallByte = HEADERBYTE + m_useByte + FOOTERBYTE;
        *footer = overallByte;
    }
    static void operator delete( void* p )
    {
        BaundaryTag* own = reinterpret_cast<BaundaryTag*>( p );

        u_int overallByte = own->getOverallByte( );
        BaundaryTag* noUseFirstItr = own;
        if ( !noUseFirstItr->IsBegin( ) )
        {
            // 自分より前の領域を探します。
            for ( BaundaryTag* itr = reinterpret_cast<BaundaryTag*>( noUseFirstItr->getPrevHeaderPointer( ) );
                  ;
                  itr = reinterpret_cast<BaundaryTag*>( itr->getPrevHeaderPointer( ) ) )
            {
                log( "prev" );
                if ( itr->IsUse( ) ) break;
                noUseFirstItr = itr;
                overallByte += itr->getOverallByte( );
                if ( itr->IsBegin( ) ) break;
            }
        }

        // 自分より先の領域を探します。
        for ( BaundaryTag* itr = reinterpret_cast<BaundaryTag*>( own->getNextHeaderPointer( ) );
              !itr->IsEnd( );
              itr = reinterpret_cast<BaundaryTag*>( itr->getNextHeaderPointer( ) ) )
        {
            log( "next" );
            if ( !itr->IsUse( ) ) overallByte += itr->getOverallByte( );
            else break;
        }

        auto noUseFirstItrisBegin = noUseFirstItr->m_state & State::BEGIN;
        memset( noUseFirstItr, 0, overallByte );

        noUseFirstItr->m_useByte = overallByte - HEADERBYTE - FOOTERBYTE;
        noUseFirstItr->m_state = noUseFirstItrisBegin;
        noUseFirstItr->setupOverallByte( );

        log( "deleted[%d]", overallByte );
    }
    bool IsUse( )
    {
        return ( m_state & State::USE ) == State::USE;
    }
    bool IsBegin( )
    {
        return( m_state & State::BEGIN ) == State::BEGIN;
    }
    bool IsEnd( )
    {
        return( m_state & State::END ) == State::END;
    }
    u_int getUseByte( )
    {
        return m_useByte;
    }
public:
    static const size_t STATEBYTE;
    static const size_t SIZEBYTE;
    static const size_t HEADERBYTE;
    static const size_t FOOTERBYTE;
private:
    char m_state = 0;
    u_int m_useByte = 0;
};
#pragma pack(pop)
const size_t BaundaryTag::STATEBYTE = sizeof( char );
const size_t BaundaryTag::SIZEBYTE = sizeof( u_int );
const size_t BaundaryTag::HEADERBYTE = BaundaryTag::STATEBYTE + BaundaryTag::SIZEBYTE;
const size_t BaundaryTag::FOOTERBYTE = sizeof( u_int );
const char BaundaryTag::State::USE = 1 << 0;
const char BaundaryTag::State::BEGIN = 1 << 1;
const char BaundaryTag::State::END = 1 << 2;
// END | BEGIN | USE
