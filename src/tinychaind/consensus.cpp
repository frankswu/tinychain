/**
 * Copyright (c) 2019-2020 CHENHAO.
 *
 * This file is part of tinychain.
 *
 * tinychain is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/**
 * Part of:
 * Comments:
 *
**/
#include <algorithm>
#include <tinychain/elements/tinychain.hpp>
#include <tinychain/organizer/consensus.hpp>
#include <tinychain/organizer/blockchain.hpp>
#include <tinychain/network/network.hpp>

namespace tinychain
{

void miner::start(address_t addr){
    while (isMining_)
    {
        block new_block;

        // 未找到，继续找
        if (!pow_once(new_block, addr)) {
            continue;
        }

        // 需要在pool中移除已经被打包的交易
        chain_.pool_reset(new_block.header_.tx_count);

        // 调用网络广播
        //ws_send(new_block.to_json().toStyledString());

        // 本地存储
        chain_.push_block(new_block);
    }
}

tx miner::create_coinbase_tx(address_t& addr) {
    // TODO
    return tx{addr};
}

bool miner::pow_once(block& new_block, address_t& addr) {

    auto&& pool = chain_.pool();

    auto&& prev_block = chain_.get_last_block();

    // 填充新块
    new_block.header_.height = prev_block.header_.height + 1;
    new_block.header_.prev_hash = prev_block.header_.hash;

    new_block.header_.timestamp = get_now_timestamp();

    new_block.header_.tx_count = pool.size();

    // 难度调整: 
    // 控制每块速度，控制最快速度，大约10秒
    uint64_t time_peroid = new_block.header_.timestamp - prev_block.header_.timestamp;
    log::info("consensus") << "block time :" << time_peroid <<" s";

    if (time_peroid <= 10u) {
        new_block.header_.difficulty = prev_block.header_.difficulty + 9000;
    } else {
        if (prev_block.header_.difficulty <= 3000) {
            new_block.header_.difficulty = prev_block.header_.difficulty + 9000;
        } else {
            new_block.header_.difficulty = prev_block.header_.difficulty - 3000;
        }
    }

    log::debug("consensus")<< prev_block.header_.difficulty;

    // 计算挖矿目标值,最大值除以难度就目标值
    uint64_t target = 0xffffffffffffffff / prev_block.header_.difficulty;

    // 设置coinbase交易, CHENHAO => 这里继续，设置coinbase，后续实现验证交易和区块。
    auto&& tx = create_coinbase_tx(addr);
    pool.push_back(tx);

    // 装载交易
    new_block.setup(pool);

    // 计算目标值
    for ( uint64_t n = 0; ; ++n) {
        //尝试候选目标值
        new_block.header_.nonce = n;
        auto&& jv_block = new_block.to_json();
        auto&& can = to_sha256(jv_block);
        uint64_t ncan = std::stoull(can.substr(0, 16), 0, 16); //截断前16位，转换uint64 后进行比较

        // 找到了
        if (ncan < target) {
            new_block.header_.hash = can;
            //log::info("consensus") << "target:" << ncan;
            //log::info("consensus") << "new block :" << can;
            log::info("consensus") << "new block :" << jv_block.toStyledString();
            return true;
        }
    }

    return false;
}

bool validate_tx(blockchain& chain, const tx& new_tx) {
    // TODO
    // input exsited?
    auto&& inputs = new_tx.inputs();
    for (auto& each : inputs) {

        tx pt;
        if (!chain.get_tx(std::get<0>(each), pt)) {
            return false;
        }

        log::info("consensus")<<pt.to_json().toStyledString();
        //auto&& tl = pt.outputs();
    }

    return true;
}

bool validate_block(const tx& new_block) {
    // TODO

    return true;
}


} //tinychain
