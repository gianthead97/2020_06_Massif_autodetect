#include <iostream>
#include <map>
#include <parser/parser.hpp>
#include <parser/rules.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>

#include <boost/fusion/sequence/intrinsic/at.hpp>
#include <boost/fusion/include/at.hpp>

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;
namespace fusion = boost::fusion;

MassifParser::MassifParser(const std::string& path)
    :mDesc(""), mCmd(""), mTimeUnit(""), mPeakSnapshot(nullptr) ,mFile(path)
{
    if (!mFile.is_open()) {
        status = ParserStatus::ePARSER_FAIL;
        std::cerr << "Fail to open " << path << "\n";
    }
    mContent << mFile.rdbuf();
}

MassifParser::~MassifParser()
{
    if (mFile.is_open()) {
        mFile.close();
    }
}


std::ostream& operator<< (std::ostream &out, const MassifParser &mp)
{
   
    for(auto snapshot: mp.mSnapshots){
        out << *snapshot.get();
    }

    return out;
}

MassifParser::ParserStatus MassifParser::parse()
{
    using boost::spirit::x3::_attr;
    std::shared_ptr<Snapshot> currentSnapshot;

    auto snapshotInfo = [&](auto& ctx)
    {
        auto attr = _attr(ctx);
        auto title = fusion::at_c<0>(attr);
        auto time = fusion::at_c<1>(attr);
        auto memHeapB = fusion::at_c<2>(attr);
        auto memHeapExtra = fusion::at_c<3>(attr);
        auto memStacks = fusion::at_c<4>(attr);
        std::string snapshotType = fusion::at_c<5>(attr);

        currentSnapshot = std::make_shared<Snapshot>(title, time, memHeapB, memHeapExtra, memStacks);
       
        if(!snapshotType.compare("peak")){
            mPeakSnapshot = currentSnapshot;
        } else {
            this->mSnapshots.push_back(currentSnapshot);
        }
    };

    auto treeHeader = [&](auto& ctx) 
    {
        auto attr = _attr(ctx);
        auto number = fusion::at_c<0>(attr);
        auto bytes = fusion::at_c<1>(attr);
        auto message = fusion::at_c<2>(attr);

        currentSnapshot->changeHeaderInfo(number, bytes, message);
    };

    auto treeNode = [&](auto& ctx) 
    {
        auto attr = _attr(ctx);

        auto number = fusion::at_c<0>(attr);
        auto bytes = fusion::at_c<1>(attr);
        auto location = fusion::at_c<2>(attr);
        auto function = fusion::at_c<3>(attr);
        auto file = fusion::at_c<4>(attr);
        auto line = fusion::at_c<5>(attr);

        currentSnapshot->addTreeNode(number,bytes,location,function,file,line);
    };

    auto extraLine = [](auto& ctx) 
    {
        auto attr = _attr(ctx);

        auto number = fusion::at_c<0>(attr);
        auto message = fusion::at_c<1>(attr);

        // std::cout << "ExtraLineNumber: " << number << std::endl 
        //           << "ExtraLineMessage: " << message << std::endl << std::endl; ; 
    };

    auto header = [&](auto& ctx)
    { 
        auto attr = _attr(ctx);
        this->mDesc = fusion::at_c<0>(attr);
        this->mCmd = fusion::at_c<1>(attr);
        this->mTimeUnit = fusion::at_c<2>(attr);
    };     

    auto const content = MassifParser::mContent.str();
    bool parseStatus = x3::parse(content.begin(),
                                content.end(),
                                massifRules::rHeader [header] >> 
                                *(massifRules::rSnapshotInfo [snapshotInfo] >> 
                                ((massifRules::rTreeHeader [treeHeader]
                                    >> *(massifRules::rTreeNode [treeNode] | massifRules::rExtraLine [extraLine])) | "")));

    return parseStatus ? ParserStatus::ePARSER_OK : ParserStatus::ePARSER_FAIL;

};



/* Dve globalne mape koja preslikavaju: 
    1. broj u naziv funkcije 
    2. broj u naziv fajla
*/

auto functionNameMap = std::map<int, std::string>();
auto fileNameMap = std::map<int, std::string>();

XtmemoryParser::XtmemoryParser(const std::string& path)
    :_file(path)
{
    if (!_file.is_open()) {
        status = ParserStatus::ePARSER_FAIL;
        std::cerr << "Fail to open " << path << "\n";
    }
    _content << _file.rdbuf();
}

XtmemoryParser::~XtmemoryParser()
{
    if (_file.is_open()) {
        _file.close();
    }
}


std::ostream& operator<< (std::ostream &out, const XtmemoryParser &xtmp)
{
   
    for(auto node: xtmp._tree._nodes){
        out << *node.get();
    }

    return out;
}

XtmemoryParser::ParserStatus XtmemoryParser::parse()
{
    using boost::spirit::x3::_attr;
    
    std::shared_ptr<Node> currentNode;
    auto const content = XtmemoryParser::_content.str();

    auto subAlloc = [&](auto& ctx)
    { 
        
        auto attr = _attr(ctx);
        auto fileNumber = fusion::at_c<0>(attr);
        auto fileName = fusion::at_c<1>(attr);
        if (fileName == ""){
            fileName = fileNameMap[fileNumber];
        } else {
            fileNameMap[fileNumber] = fileName;
        }

        auto functionNumber =  fusion::at_c<2>(attr);
        auto functionName = fusion::at_c<3>(attr);  
        if (functionName == ""){
            functionName = functionNameMap[fileNumber];
        } else {
            functionNameMap[functionNumber] = functionName;
        }

        auto lineNumber =  fusion::at_c<4>(attr);

        auto cFi = fusion::at_c<5>(attr);
        auto cFiName = fusion::at_c<6>(attr);
        auto cFn = fusion::at_c<7>(attr);
        auto cFnName = fusion::at_c<8>(attr);

        if (cFiName == ""){
            cFiName = fileNameMap[cFi];
        } else {
            fileNameMap[cFi] = cFiName;
        }

        if (cFnName == ""){
            cFnName = functionNameMap[cFn];
        } else {
            functionNameMap[cFn] = cFnName;
        }

        auto calls1 =  fusion::at_c<9>(attr);
        auto calls2 = fusion::at_c<10>(attr);
      
        auto allocation =  std::vector<int>();
        allocation.push_back(fusion::at_c<12>(attr));
        allocation.push_back(fusion::at_c<13>(attr));
        allocation.push_back(fusion::at_c<14>(attr));
        allocation.push_back(fusion::at_c<15>(attr));
        allocation.push_back(fusion::at_c<16>(attr));
        allocation.push_back(fusion::at_c<17>(attr));        
        
        auto child = std::make_shared<Node>(cFiName, cFnName, calls2, std::vector<int>());
        auto node = std::make_shared<Node>(fileName, functionName, lineNumber, allocation);
        node->setChild(child);

        this->_tree.addNode(node);
    };   

    auto directAlloc = [&](auto& ctx)
    { 
        auto attr = _attr(ctx);
        auto fileNumber = fusion::at_c<0>(attr);
        auto fileName = fusion::at_c<1>(attr);
        if (fileName == ""){
            fileName = fileNameMap[fileNumber];
        } else {
            fileNameMap[fileNumber] = fileName;
        }

        auto functionNumber =  fusion::at_c<2>(attr);
        auto functionName = fusion::at_c<3>(attr);  
        if (functionName == ""){
            functionName = functionNameMap[functionNumber];
        } else {
            functionNameMap[functionNumber] = functionName;
        }

        int lineNumber = fusion::at_c<4>(attr);

        auto allocation =  std::vector<int>();
        allocation.push_back(fusion::at_c<5>(attr));
        allocation.push_back(fusion::at_c<6>(attr));
        allocation.push_back(fusion::at_c<7>(attr));
        allocation.push_back(fusion::at_c<8>(attr));
        allocation.push_back(fusion::at_c<9>(attr));
        allocation.push_back(fusion::at_c<10>(attr));        
        
        auto node = std::make_shared<Node>(fileName, functionName, lineNumber, allocation);
        this->_tree.addNode(node);
    };
    
    auto totals = [&](auto& ctx)
    {
        auto attr = _attr(ctx);

        this->_tree._totals.push_back(fusion::at_c<0>(attr));
        this->_tree._totals.push_back(fusion::at_c<1>(attr));
        this->_tree._totals.push_back(fusion::at_c<2>(attr));
        this->_tree._totals.push_back(fusion::at_c<3>(attr));
        this->_tree._totals.push_back(fusion::at_c<4>(attr));
        this->_tree._totals.push_back(fusion::at_c<5>(attr));
    };

    bool parseStatus = x3::parse(content.begin(), 
                                content.end(),
                                xtmemoryRules::rHeader >> 
                                +(*(xtmemoryRules::rSubAlloc [subAlloc]) >> 
                                xtmemoryRules::rDirectAlloc [directAlloc] >> *("\n")) >> 
                                xtmemoryRules::rTotals);

    return parseStatus ? ParserStatus::ePARSER_OK : ParserStatus::ePARSER_FAIL;

};

