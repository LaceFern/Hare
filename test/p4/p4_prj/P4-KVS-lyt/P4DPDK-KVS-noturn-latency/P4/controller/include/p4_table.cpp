//
// Created by Alex_Li on 2022/7/6.
//

#include "p4_table.h"

P4Table::P4Table(const P4Program &program, const bfrt::BfRtTable *table) : associated_program(program), table(table)
{
	table->keyAllocate(&key_handler);
	table->dataAllocate(&data_handler);

	table->tableSizeGet(*associated_program.session, associated_program.device_target, 0, &table_size);
}

void P4Table::setTable(const std::string &table_name)
{
	bf_status_t status = associated_program.bfrt_info->bfrtTableFromNameGet(table_name, &table);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(table_name + ": No such table");
	}
}

void P4Table::setAction(const std::string &action_name)
{
	bf_status_t status = table->actionIdGet(action_name, &actions[action_name]);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(action_name + ": No such action");
	}
}

bf_rt_id_t P4Table::getAction(const std::string &action_name)
{
	if(actions.find(action_name) == actions.cend())
	{
		setAction(action_name);
	}
	return actions.at(action_name);
}

void P4Table::setKey(const std::string &key_name)
{
	bf_status_t status = table->keyFieldIdGet(key_name, &keys[key_name]);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(key_name + ": No such key");
	}
}

bf_rt_id_t P4Table::getKey(const std::string &key_name)
{
	if(keys.find(key_name) == keys.cend())
	{
		setKey(key_name);
	}
	return keys.at(key_name);
}

void P4Table::setData(const std::string &data_name)
{
	bf_status_t status = table->dataFieldIdGet(data_name, &data[data_name]);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(data_name + ": No such data");
	}
}

bf_rt_id_t P4Table::getData(const std::string &data_name)
{
	if(data.find(data_name) == data.cend())
	{
		setData(data_name);
	}
	return data.at(data_name);
}

void P4Table::setData(const std::string &data_name, bf_rt_id_t action)
{
	bf_status_t status = table->dataFieldIdGet(data_name, action, &data[data_name]);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(data_name + ": No such data");
	}
}

bf_rt_id_t P4Table::getData(const std::string &data_name, bf_rt_id_t action)
{
	if(data.find(data_name) == data.cend())
	{
		setData(data_name, action);
	}
	return data.at(data_name);
}

void P4Table::setData(const std::string &data_name, const std::string &action)
{
	bf_status_t status = table->dataFieldIdGet(data_name, getAction(action), &data[action + '.' + data_name]);
	if(status != BF_SUCCESS)
	{
		throw std::out_of_range(action + '.' + data_name + ": No such data");
	}
}

bf_rt_id_t P4Table::getData(const std::string &data_name, const std::string &action)
{
	if(data.find(action + '.' + data_name) == data.cend())
	{
		setData(data_name, action);
	}
	return data.at(action + '.' + data_name);
}

void P4Table::initActions()
{
	std::vector<bf_rt_id_t> action_id;
	BF_TRY(table->actionIdListGet(&action_id));
	for(auto ai : action_id)
	{
		std::string action_name;
		BF_TRY(table->actionNameGet(ai, &action_name));
		actions[action_name] = ai;
	}
	for(const auto &a : actions)
	{
		std::vector<bf_rt_id_t> data_id;
		BF_TRY(table->dataFieldIdListGet(a.second, &data_id));
		for(auto di : data_id)
		{
			std::string data_name;
			BF_TRY(table->dataFieldNameGet(di, a.second, &data_name));
			data[a.first + '.' + data_name] = di;
		}
	}
}

void P4Table::initKeys()
{
	std::vector<bf_rt_id_t> key_id;
	BF_TRY(table->keyFieldIdListGet(&key_id));
	for(auto ki : key_id)
	{
		std::string key_name;
		BF_TRY(table->keyFieldNameGet(ki, &key_name));
		keys[key_name] = ki;
	}
}

void P4Table::initData()
{
	std::vector<bf_rt_id_t> data_id;
	BF_TRY(table->dataFieldIdListGet(&data_id));
	for(auto di : data_id)
	{
		std::string data_name;
		BF_TRY(table->dataFieldNameGet(di, &data_name));
		data[data_name] = di;
	}
}

void P4Table::initEntries()
{
	table_entries.resize(table_size);
	entry_pointers.clear();
	for(auto &p : table_entries)
	{
		BF_TRY(table->keyAllocate(&p.first));
		BF_TRY(table->dataAllocate(&p.second));
		entry_pointers.emplace_back(p.first.get(), p.second.get());
	}
}

void P4Table::setOperation(bfrt::TableOperationsType type)
{
	BF_TRY(table->operationsAllocate(type, &table_operations));
	BF_TRY(table_operations->registerSyncSet(*associated_program.session, associated_program.device_target, nullptr, nullptr));

//	BF_TRY(table->operationsAllocate(bfrt::TableOperationsType::COUNTER_SYNC, &table_operations));
//	BF_TRY(table_operations->counterSyncSet(*associated_program.session, associated_program.dev_target, nullptr, nullptr));
}

void P4Table::syncSoftwareShadow()
{
	BF_TRY(table->tableOperationsExecute(*table_operations));
}

uint32_t P4Table::tableGet(uint64_t flags)
{
	BF_TRY(table->tableEntryGetFirst(
			*associated_program.session,
			associated_program.device_target,
			flags,
			entry_pointers[0].first,
			entry_pointers[0].second
	));
	uint32_t ret = 0;
	BF_TRY(table->tableEntryGetNext_n(
			*associated_program.session,
			associated_program.device_target,
			flags,
			*entry_pointers[0].first,
			table_size,
			&entry_pointers,
			&ret
	));
	return ret;
}

const bfrt::BfRtTable::keyDataPairs& P4Table::tableDateGet()
{
	return entry_pointers;
}

void P4Table::entryGet(int index)
{
	BF_TRY(entry_pointers[index].first->setValue(keys[""], "123"));
	BF_TRY(table->tableEntryGet(
			*associated_program.session,
			associated_program.device_target,
			*entry_pointers[index].first,
			bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW,
			entry_pointers[index].second
	));
}

void P4Table::addEntry(const std::unordered_map<bf_rt_id_t, uint64_t> &key, const std::unordered_map<bf_rt_id_t, uint64_t> &value)
{
	table->keyReset(key_handler.get());
	for(const auto &k : key)
	{
		key_handler->setValue(k.first, k.second);
	}

	table->dataReset(data_handler.get());
	for(const auto &v : value)
	{
		data_handler->setValue(v.first, v.second);
	}

	table->tableEntryAdd(*associated_program.session, associated_program.device_target, *key_handler, *data_handler);

	completeOperations();
}

void P4Table::addEntry(const std::unordered_map<bf_rt_id_t, uint64_t> &key, const std::unordered_map<bf_rt_id_t, uint64_t> &value, bf_rt_id_t action)
{
	table->keyReset(key_handler.get());
	for(const auto &k : key)
	{
		key_handler->setValue(k.first, k.second);
	}

	table->dataReset(action, data_handler.get());
	for(const auto &v : value)
	{
		data_handler->setValue(v.first, v.second);
	}

	table->tableEntryAdd(*associated_program.session, associated_program.device_target, 0, *key_handler, *data_handler);
}

//template<typename K, typename V>
//void P4Table::addEntry(const std::unordered_map<bf_rt_id_t, K> &key, const std::unordered_map<bf_rt_id_t, V> &value)
//{
//	table->keyReset(key_handler.get());
//	for(const auto &k : key)
//	{
//		key_handler->setValue(k.first, k.second);
//	}
//
//	table->dataReset(data_handler.get());
//	for(const auto &v : value)
//	{
//		data_handler->setValue(v.first, v.second);
//	}
//
//	table->tableEntryAdd(*associated_program.session, associated_program.dev_target, *key_handler, *data_handler);
//}

//template<typename K, typename V>
//void P4Table::addEntry(const std::unordered_map<bf_rt_id_t, K> &key, const std::unordered_map<bf_rt_id_t, V> &value, bf_rt_id_t action)
//{
//	table->keyReset(key_handler.get());
//	for(const auto &k : key)
//	{
//		key_handler->setValue(k.first, k.second);
//	}
//
//	table->dataReset(action, data_handler.get());
//	for(const auto &v : value)
//	{
//		data_handler->setValue(v.first, v.second);
//	}
//
//	table->tableEntryAdd(*associated_program.session, associated_program.dev_target, *key_handler, *data_handler);
//}

void P4Table::addEntries(const std::vector<std::pair<std::unordered_map<bf_rt_id_t, uint64_t>, std::unordered_map<bf_rt_id_t, uint64_t>>> &entries)
{
	beginBatch();
	for(const auto &e : entries)
	{
		addEntry(e.first, e.second);
	}
	endBatch(true);
}

void P4Table::addEntries(const std::vector<std::pair<std::unordered_map<bf_rt_id_t, uint64_t>, std::unordered_map<bf_rt_id_t, uint64_t>>> &entries, bf_rt_id_t action)
{
	for(const auto &e : entries)
	{
		addEntry(e.first, e.second, action);
	}
}

//template<typename... key_args, typename... data_args>
//void P4Table::modifyEntry(std::tuple<key_args...> t, std::tuple<>)
//{
//	table->keyReset(key_handler.get());
//	for(const auto &k : key)
//	{
//		key_handler->setValue(k.first, k.second);
//	}
//	key_handler->setValue(0, std::get<0>(t));
//
//	table->dataReset(data_handler.get());
//	for(const auto &v : t)
//	{
//		data_handler->setValue(v.first, v.second);
//	}
//
//	table->tableEntryAdd(*associated_program.session, associated_program.dev_target, 0, *key_handler, *data_handler);
//}

