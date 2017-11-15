//
//  address.hpp
//  blocksci
//
//  Created by Harry Kalodner on 7/12/17.
//
//

#ifndef address_hpp
#define address_hpp

#include "address_types.hpp"

#include <blocksci/chain/chain_fwd.hpp>

#include <range/v3/utility/optional.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace blocksci {
    struct Script;
    class AnyScript;
    class ScriptAccess;
    class HashIndex;
    class AddressIndex;
    struct DataConfiguration;
    
    struct Address {
        
        uint32_t scriptNum;
        AddressType::Enum type;
        
        Address();
        Address(uint32_t addressNum, AddressType::Enum type);
        
        bool isSpendable() const;
        
        bool operator==(const Address& other) const {
            return type == other.type && scriptNum == other.scriptNum;
        }
        
        bool operator!=(const Address& other) const {
            return !operator==(other);
        }
        
        bool operator!=(const Script &other) const;
        
        std::string toString() const;
        
        AnyScript getScript(const ScriptAccess &access) const;
        
        std::vector<const Output *> getOutputs(const AddressIndex &index, const ChainAccess &chain) const;
        std::vector<const Input *> getInputs(const AddressIndex &index, const ChainAccess &chain) const;
        std::vector<Transaction> getTransactions(const AddressIndex &index, const ChainAccess &chain) const;
        std::vector<Transaction> getOutputTransactions(const AddressIndex &index, const ChainAccess &chain) const;
        std::vector<Transaction> getInputTransactions(const AddressIndex &index, const ChainAccess &chain) const;
        

        std::string fullType(const ScriptAccess &script) const;

        // Requires DataAccess
        #ifndef BLOCKSCI_WITHOUT_SINGLETON        
        AnyScript getScript() const;
        
        std::vector<const Output *> getOutputs() const;
        std::vector<const Input *> getInputs() const;
        std::vector<Transaction> getTransactions() const;
        std::vector<Transaction> getOutputTransactions() const;
        std::vector<Transaction> getInputTransactions() const;

        std::string fullType() const;
        #endif
    };
    
    size_t addressCount(const ScriptAccess &access);
    
    void visit(const Address &address, const std::function<bool(const Address &)> &visitFunc, const ScriptAccess &access);
    
    ranges::optional<Address> getAddressFromString(const DataConfiguration &config, const HashIndex &index, const std::string &addressString);
    
    // Requires DataAccess
    #ifndef BLOCKSCI_WITHOUT_SINGLETON
    ranges::optional<Address> getAddressFromString(const std::string &addressString);
    size_t addressCount();
    #endif
}

std::ostream &operator<<(std::ostream &os, const blocksci::Address &address);

namespace std {
    template <>
    struct hash<blocksci::Address> {
        typedef blocksci::Address argument_type;
        typedef size_t  result_type;
        result_type operator()(const argument_type &b) const {
            return (static_cast<size_t>(b.scriptNum) << 32) + static_cast<size_t>(b.type);
        }
    };
}

#endif /* address_hpp */
