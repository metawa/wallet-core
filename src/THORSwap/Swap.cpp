// Copyright © 2017-2021 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include "Swap.h"

#include <TrustWalletCore/TWCoinType.h>
#include "Coin.h"
#include "../proto/Binance.pb.h"

#include <iostream>

/*
 * References:
 *  https://gitlab.com/thorchain/asgardex-common/asgardex-util
 */

namespace TW::THORSwap {

TWCoinType chainCoinType(Chain chain) {
    switch (chain) {
        //case Chain::THOR: return TWCoinTypeBitcoin; // TODO
        case Chain::BTC: return TWCoinTypeBitcoin;
        case Chain::ETH: return TWCoinTypeEthereum;
        case Chain::BNB: return TWCoinTypeBinance;
        default: return TWCoinTypeBitcoin;
    }
}

std::string chainName(Chain chain) {
    switch (chain) {
        case Chain::THOR: return "THOR";
        case Chain::BTC: return "BTC";
        case Chain::ETH: return "ETH";
        case Chain::BNB: return "BNB";
        default: return "";
    }
}

std::string Swap::buildMemo(Chain toChain, const std::string& toSymbol, const std::string& toAddress, uint64_t limit) {
    return "SWAP:" + chainName(toChain) + "." + toSymbol + ":" + toAddress + ":" + std::to_string(limit);
}

bool validateAddress(Chain chain, const std::string& address) {
    return TW::validateAddress(chainCoinType(chain), address);
}

std::pair<Data, std::string> Swap::build(
    Chain fromChain,
    Chain toChain,
    const std::string& fromAddress,
    const std::string& toSymbol,
    const std::string& toTokenId,
    const std::string& toAddress,
    const std::string& vaultAddress,
    const std::string& amount
)  {
    auto fromCoin = chainCoinType(fromChain);

    if (!validateAddress(fromChain, fromAddress)) {
        return std::make_pair<Data, std::string>({}, "Invalid own address");
    }

    uint64_t limit = 343050111; // TODO
    auto memo = buildMemo(toChain, toSymbol, toAddress, limit);

    switch (fromChain) {
        case Chain::BNB: {
            Data out;
            auto res = buildBinance(toChain, toSymbol, toTokenId, fromAddress, toAddress, vaultAddress, amount, memo, out);
            return std::make_pair(std::move(out), std::move(res));
        }

        default:
            return std::make_pair<Data, std::string>({}, "Invalid from chain: " + std::to_string(toChain));
    }
}

std::string Swap::buildBinance(Chain toChain, const std::string& toSymbol, const std::string& toTokenId, const std::string& fromAddress, const std::string& toAddress, const std::string& vaultAddress, const std::string& amount, const std::string& memo, Data& out) {
    auto input = Binance::Proto::SigningInput();

    input.set_chain_id("Binance-Chain-Nile");
    input.set_account_number(0);
    input.set_sequence(0);
    input.set_source(0);

    input.set_memo(memo);

    auto& order = *input.mutable_send_order();

    auto token = Binance::Proto::SendOrder::Token();
    token.set_denom("BNB");
    token.set_amount(1000000000000); // TODO amount from input string
    {
        auto input = order.add_inputs();
        input->set_address(fromAddress);
        *input->add_coins() = token;
    }
    {
        auto output = order.add_outputs();
        output->set_address(vaultAddress);
        *output->add_coins() = token;
    }

    auto serialized = input.SerializeAsString();
    out.insert(out.end(), serialized.begin(), serialized.end());
    return "";
}

} // namespace TW
