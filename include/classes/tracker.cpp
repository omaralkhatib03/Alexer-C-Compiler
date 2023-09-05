#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <map> 
#include <bitset>


typedef std::map<std::string, std::pair<int, int>> ScopeToBytes; // id, allocatedBytes, freeBytes
typedef std::map<std::string, std::string> TypeDefTable; // unused


struct Tracker 
{
    Tracker(){}


    void addScope(std::string _id) // adds function to stack (entering new scope)
    {
        idToBytesMap[_id] = std::make_pair(16, 0);
        currentScopeId = _id;
    }

    void incrementOffset(std::string _id) // add memory to current scope more needed
    {
        idToBytesMap[_id].first += 16;
    }

    // add memory to current scope more needed
    void setFreeBytes(std::string _id, int freeBytes) // needed since its not always going to be 16
    {
        idToBytesMap[_id].second = freeBytes;
    }

    int getOffset(std::string _id) // get offset of current scope
    {
        return idToBytesMap[_id].first;
    }

    int getFreeBytes(std::string _id) // get free bytes of current scope
    {
        return idToBytesMap[_id].second;
    }

    void decrementFreeBytes(std::string _id, int size)
    {
        idToBytesMap[_id].second -= size;
    }

    bool enoughFreeBytes(std::string _id, int size)
    {
        return idToBytesMap[_id].second >= size;
    }

    int bytesNeeded(std::string _id, int size)
    {
        return size - this->getFreeBytes(_id);
    }

    // void addSymbol(std::string _id, bool t) // add symbol to current scope // unused
    // {
    //     symbolAllocated[_id] = t;
    // }

    // bool symbolExists(std::string _id) // check if symbol exists in current scope  //unused
    // {
    //     return symbolAllocated[_id];
    // }

    std::string getCurrentScopeId()
    {
        return currentScopeId;
    }


    void allocateMemory(std::string _id, int size) // allocate memory for symbol
    {   

        // std::cout << "allocating memory for " << _id << " with size " << size << std::endl;
        int bytesNeeded = this->bytesNeeded(_id, size);
        // std::cout << "bytes needed: " << bytesNeeded << std::endl;
        // std::cout << "before allocing: "<< std::endl;
        // std::cout << _id << " " << idToBytesMap[_id].first << " " << idToBytesMap[_id].second << std::endl;
        
        if (bytesNeeded > 0) // i need more memory
        {
            if (bytesNeeded % 16 != 0) // more memory of a specific ammount, not 16 aligned
            {
                // std::cout << "idToBytesMap[_id].first before: " << idToBytesMap[_id].first << std::endl;
                idToBytesMap[_id].first += (16 - ((bytesNeeded % 16) % 16)) + bytesNeeded;
                // std::cout << "idToBytesMap[_id].first after: " << idToBytesMap[_id].first << std::endl;
                this->setFreeBytes(_id,  (16 - ((bytesNeeded % 16) % 16)));
                return;
            }
            else // algorithm ensures that alloced memory is 1+ that multiple of bytesNeeded
            {
                // alloc bytesNeeded + 16
                idToBytesMap[_id].first += (16 - ((bytesNeeded % 16) % 16)) + bytesNeeded;
                this->setFreeBytes(_id, 16); // set free bytes to 16
                return;
            }
        }
        else if (bytesNeeded < 0) // enough free bytes, just decrement by the used bytes
        {
            this->setFreeBytes(_id, this->getFreeBytes(_id) - size);
            return;
        }
        else // bytesNeeded == 0,  used up all free bytes, alloc 16 and set them as free
        {
            this->incrementOffset(_id);
            this->setFreeBytes(_id, 16);
            return;
        }
    }

    void printAll(std::ostream &out)
    {
        for (auto it = idToBytesMap.begin(); it != idToBytesMap.end(); it++)
        {
            out << it->first << " " << it->second.first << " " << it->second.second << std::endl;
        }
    }

    void resetScope()
    {
        currentScopeId = "global";
    }

    std::map<std::string, int> createCtxTable ()
    {
        std::map<std::string, int> ctxTable;
        for (auto it = idToBytesMap.begin(); it != idToBytesMap.end(); it++)
        {
            ctxTable[it->first] = it->second.first;
        }
        return ctxTable;
    }

    void setScopeAsCaller(std::string _id)
    {
        scopeCallerCallee[_id][0] = 1;
    }

    void setScopeAsCallee(std::string _id)
    {
        scopeCallerCallee[_id][1] = 1;
    }

    bool isCaller(std::string _id)
    {
        return scopeCallerCallee[_id][0];
    }

    bool isCallee(std::string _id)
    {
        return scopeCallerCallee[_id][1];
    }

    void printCallerCallee(std::ostream &out)
    {
        for (auto it = scopeCallerCallee.begin(); it != scopeCallerCallee.end(); it++)
        {
            out << it->first << " " << it->second[0] << " " << it->second[1] << std::endl;
        }
    }

    std::map<std::string, std::bitset<2>> getFunctionCallTypes()
    {
        return scopeCallerCallee;
    } 

    void addTypedef(std::string _id, std::string _type)
    {
        typedefTable[_id] = _type;
    }

    bool isTypedef(std::string _id)
    {
        try
        {
            typedefTable.at(_id);
            return true;
        }
        catch(const std::exception& e)
        {
            return false;  
        } 
        
    }

    std::string getTypedefType(std::string _id)
    {
        return typedefTable.at(_id);
    }
    

    void printTypeDefs()
    {
        for (auto it = typedefTable.begin(); it != typedefTable.end(); it++)
        {
            std::cout << it->first << " " << it->second << std::endl;
        }
    }

    void addFunctionParams(std::string _id, std::vector<std::string> _params)
    {
        functionParams[_id] = _params;
    }

    std::vector<std::string> getFunctionParams(std::string _id)
    {
        return functionParams.at(_id);
    }

    void printFunctionParams()
    {
        for (auto it = functionParams.begin(); it != functionParams.end(); it++)
        {
            std::cout << it->first << " ";
            for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
            {
                std::cout << *it2 << " ";
            }
            std::cout << std::endl;
        }
    }

    ~Tracker(){}

    ScopeToBytes idToBytesMap;
    std::string currentScopeId = "global";
    std::map<std::string, std::bitset<2>> scopeCallerCallee; //[0, 0], bit 0 is caller, bit 1 is callee
    TypeDefTable typedefTable; 
    std::map<std::string, std::vector<std::string>> functionParams;
};

