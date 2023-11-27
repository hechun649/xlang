#pragma once

#include <string>
#include <unordered_map>
#include <string.h>
#include "xlstream.h"
#include "value.h"
#include "xlang.h"

namespace X 
{
    namespace AST { class Scope; }
    class XlangRuntime;
    class XLangStreamException
        : public XLangException
    {
    public:
        XLangStreamException(int code)
        {
            m_code = code;
        }
        virtual const char* what() const throw()
        {
            return "XLangStream exception happened";
        }
    };
    class XScopeSpace
    {
        friend class XLangStream;
        std::unordered_map<unsigned long long, void*> m_map;
        AST::Scope* m_curScope = nullptr;
        //for function inside class, we need to record class scope to
        //let this.item get the right scope
        //used for checking if it is extern or not
        //for this.item, item will not be extern

        AST::Scope* m_curClassScope = nullptr;
        XlangRuntime* m_rt = nullptr;
        XObj* m_pContext = nullptr;
    public:
        void SetCurrentScope(AST::Scope* p)
        {
            m_curScope = p;
        }
        void SetCurrentClassScope(AST::Scope* p)
        {
            m_curClassScope = p;
        }
        void SetContext(XlangRuntime* rt, XObj* pContext)
        {
            m_rt = rt;
            m_pContext = pContext;
        }
        XObj* SetContext(XObj* pContext)
        {
            auto* pKeep = m_pContext;
            m_pContext = pContext;
            return pKeep;
        }
        XlangRuntime* RT() { return m_rt; }
        XObj* Context() { return m_pContext; }
        AST::Scope* GetCurrentScope()
        {
            return m_curScope;
        }
        AST::Scope* GetCurrentClassScope()
        {
            return m_curClassScope;
        }
        void Add(unsigned long long id, void* addr)
        {
            m_map.emplace(std::make_pair(id, addr));
        }
        void* Query(unsigned long long id)
        {
            auto it = m_map.find(id);
            if (it != m_map.end())
            {
                return it->second;
            }
            else
            {
                return nullptr;
            }
        }
    };
    class XLangStream :
        public XLStream
    {
        bool m_bOwnScopeSpace = false;
        XScopeSpace* m_scope_space = nullptr;
    public:
        XLangStream();
        ~XLangStream();

        void UserRefScopeSpace(XScopeSpace* pRef)
        {
            if ((m_scope_space != nullptr) && m_bOwnScopeSpace)
            {
                delete m_scope_space;
            }
            m_scope_space = pRef;
            m_bOwnScopeSpace = false;
        }
        XScopeSpace& ScopeSpace() { return *m_scope_space; }
        void SetProvider(XLStream* p)
        {
            XLangStream* pXlangStream = dynamic_cast<XLangStream*>(p);
            if (pXlangStream)
            {
                if ((m_scope_space != nullptr) && m_bOwnScopeSpace)
                {
                    delete m_scope_space;
                    m_bOwnScopeSpace = false;
                }
                m_scope_space = &pXlangStream->ScopeSpace();
            }
            m_pProvider = p;
            SetPos(p->GetPos());
        }
        void ResetPos()
        {
            curPos = { 0,0 };
        }
        virtual STREAM_SIZE Size() override
        {
            return m_size;
        }
        virtual bool FullCopyTo(char* buf, STREAM_SIZE bufSize) override;
        bool CopyTo(char* buf, STREAM_SIZE size);
        bool appendchar(char c);
        bool fetchchar(char& c);
        bool fetchstring(std::string& str);
        bool append(char* data, STREAM_SIZE size);
        FORCE_INLINE bool Skip(STREAM_SIZE size)
        {
            return CopyTo(nullptr, size);
        }
        template<typename T>
        XLangStream& operator<<(T v)
        {
            append((char*)&v, sizeof(v));
            return *this;
        }
        XLangStream& operator<<(const char c)
        {
            appendchar(c);
            return *this;
        }
        XLangStream& operator<<(std::string v)
        {
            int size = (int)v.size() + 1;
            append((char*)v.c_str(), size);
            return *this;
        }
        XLangStream& operator<<(const char* str)
        {
            int size = (int)strlen(str) + 1;
            append((char*)str, size);
            return *this;
        }
        XLangStream& operator<<(X::Value v);
        XLangStream& operator>>(X::Value& v);
        template<typename T>
        XLangStream& operator>>(T& v)
        {
            CopyTo((char*)&v, sizeof(v));
            return *this;
        }
        XLangStream& operator>>(std::string& v)
        {
            fetchstring(v);
            return *this;
        }
        XLangStream& operator>>(char& c)
        {
            fetchchar(c);
            return *this;
        }
        unsigned long long GetKey()
        {
            return m_streamKey;
        }
        FORCE_INLINE virtual blockIndex GetPos() override
        {
            return curPos;
        }
        FORCE_INLINE virtual void SetPos(blockIndex pos) override
        {
            curPos = pos;
            m_size = CalcSize(curPos);
        }
        STREAM_SIZE CalcSize(blockIndex pos);
        STREAM_SIZE CalcSize();
        void SetOverrideMode(bool b)
        {
            m_InOverrideMode = b;
        }
        bool IsEOS()
        {
            if ((BlockNum() - 1) == curPos.blockIndex)
            {
                blockInfo& blk = GetBlockInfo(curPos.blockIndex);
                return (blk.data_size == curPos.offset);
            }
            else
            {
                return false;
            }
        }
        virtual bool CanBeOverrideMode()
        {
            return true;
        }
        virtual void ReInit()
        {
            m_streamKey = 0;
            curPos = { 0,0 };
            m_size = 0;
            m_InOverrideMode = false;

            if ((m_scope_space != nullptr) && m_bOwnScopeSpace)
            {
                delete m_scope_space;
                m_scope_space = new XScopeSpace();
            }

        }
    protected:
        XLStream* m_pProvider = nullptr;//real impl.
        unsigned long long m_streamKey = 0;
        blockIndex curPos = { 0,0 };
        STREAM_SIZE m_size = 0;
        bool m_InOverrideMode = false;

        // Inherited via JitStream
        virtual void Refresh() override;
        virtual int BlockNum() override;
        virtual blockInfo& GetBlockInfo(int index) override;
        virtual bool NewBlock() override;
        virtual bool MoveToNextBlock() override;
    };
}
