//
// Created by Alex_Li on 2022/7/6.
//

#ifndef MYP4RUNTIME_P4_TABLE_H
#define MYP4RUNTIME_P4_TABLE_H


#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_operations.hpp>

#include "p4_program.h"

#define BF_TRY(x) assert((x) == BF_SUCCESS)

class P4Table
{
	const P4Program &associated_program;

public:
//	P4Table() = default;
	P4Table(const P4Program &program, const bfrt::BfRtTable *table);

public:
	void setTable(const std::string &table_name);
	void setAction(const std::string &action_name);
	void setKey(const std::string &key_name);
	void setData(const std::string &data_name);
	void setData(const std::string &data_name, bf_rt_id_t action);
	void setData(const std::string &data_name, const std::string &action);

public:
//	void getTable(const std::string &table_name));
	bf_rt_id_t getAction(const std::string &action_name);
	bf_rt_id_t getKey(const std::string &key_name);
	bf_rt_id_t getData(const std::string &data_name);
	bf_rt_id_t getData(const std::string &data_name, bf_rt_id_t action);
	bf_rt_id_t getData(const std::string &data_name, const std::string &action);

private:
	void initActions();
	void initKeys();
	void initData();

private:
	const bfrt::BfRtTable *table;
	std::unordered_map<std::string, bf_rt_id_t> actions;
	std::unordered_map<std::string, bf_rt_id_t> keys;
	std::unordered_map<std::string, bf_rt_id_t> data;

public:
	void setOperation(bfrt::TableOperationsType type);
	void syncSoftwareShadow();

private:
	std::unique_ptr<bfrt::BfRtTableOperations> table_operations;

public:
	void initEntries();

public:
	uint32_t tableGet(uint64_t flags = 0);
	const bfrt::BfRtTable::keyDataPairs& tableDateGet();

	void entryGet(int index);
	template<typename T>
	void get(const std::string &data_name, int index, T ret);

private:
	size_t table_size{};
	bfrt::BfRtTable::keyDataPairs entry_pointers;
//	std::vector<unique_ptr<bfrt::BfRtTableKey>> keys;
//	std::vector<unique_ptr<bfrt::BfRtTableData>> data;
	std::vector<std::pair<std::unique_ptr<bfrt::BfRtTableKey>, std::unique_ptr<bfrt::BfRtTableData>>> table_entries;

public:
	inline bf_status_t tableClear() const { return table->tableClear(*associated_program.session, associated_program.device_target, 0); }

	inline bf_status_t completeOperations() const { return associated_program.completeOperations(); }
	inline bf_status_t beginBatch() const { return associated_program.beginBatch(); }
	inline bf_status_t flushBatch() const { return associated_program.flushBatch(); }
	inline bf_status_t endBatch(bool hardwareSynchronous) const { return associated_program.endBatch(hardwareSynchronous); }

public:
	void addEntry(const std::unordered_map<bf_rt_id_t, uint64_t> &key, const std::unordered_map<bf_rt_id_t, uint64_t> &value);
	void addEntry(const std::unordered_map<bf_rt_id_t, uint64_t> &key, const std::unordered_map<bf_rt_id_t, uint64_t> &value, bf_rt_id_t action);
//	void addEntry(const std::pair<std::vector<bf_rt_id_t>, std::vector<bf_rt_id_t>> &header, const std::pair<std::vector<uint64_t>, std::vector<uint64_t>> &entry);
	void addEntries(const std::vector<std::pair<std::unordered_map<bf_rt_id_t, uint64_t>, std::unordered_map<bf_rt_id_t, uint64_t>>> &entries);
	void addEntries(const std::vector<std::pair<std::unordered_map<bf_rt_id_t, uint64_t>, std::unordered_map<bf_rt_id_t, uint64_t>>> &entries, bf_rt_id_t action);

//	template<typename... args>
//	void modifyEntry(std::tuple<args...> t);

public: // legacy
	inline bf_status_t keyHandlerReset() const { return table->keyReset(key_handler.get()); }

	inline bf_status_t dataHandlerReset() const { return table->dataReset(data_handler.get()); }
	inline bf_status_t dataHandlerReset(bf_rt_id_t action_id) const { return table->dataReset(action_id, data_handler.get()); }

	template<class T>
	inline bf_status_t keyHandlerSetValue(bf_rt_id_t key_id, T value) const { return key_handler->setValue(key_id, value); }
	inline bf_status_t keyHandlerSetValue(bf_rt_id_t key_id, const uint8_t *value, const size_t &size) const { return key_handler->setValue(key_id, value, size); }
	inline bf_status_t keyHandlerSetValueAndMask(bf_rt_id_t key_id, uint64_t value, uint64_t mask) const { return key_handler->setValueandMask(key_id, value, mask); }
	inline bf_status_t keyHandlerSetValueRange(bf_rt_id_t key_id, uint64_t start, uint64_t end) const { return key_handler->setValueRange(key_id, start, end); }

	template<class T>
	inline bf_status_t dataHandlerSetValue(bf_rt_id_t data_id, T value) const { return data_handler->setValue(data_id, value); }

	inline bf_status_t tableEntryAdd() const { return table->tableEntryAdd(*associated_program.session, associated_program.device_target, 0, *key_handler, *data_handler); }
	inline bf_status_t tableEntryModify() const { return table->tableEntryMod(*associated_program.session, associated_program.device_target, 0, *key_handler, *data_handler); }
	inline bf_status_t tableEntryDelete() const { return table->tableEntryDel(*associated_program.session, associated_program.device_target, 0, *key_handler); }

private:
	std::unique_ptr<bfrt::BfRtTableKey> key_handler;
	std::unique_ptr<bfrt::BfRtTableData> data_handler;

public: // legacy
	inline bf_status_t tableEntryGet() const { return table->tableEntryGet(*associated_program.session, associated_program.device_target, 0, *key_handler, data_handler.get()); }
	inline bf_status_t tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag flag) const { return table->tableEntryGet(*associated_program.session, associated_program.device_target, *key_handler, flag, data_handler.get()); }

	template<class T>
	inline bf_status_t getValue(bf_rt_id_t data_id, T *value) const { return data_handler->getValue(data_id, value); }
};

template<typename T>
void P4Table::get(const std::string &data_name, int index, T ret)
{
	entryGet(index);
	BF_TRY(entry_pointers[index].second->getValue(data.at(data_name), &ret));
}


#endif //MYP4RUNTIME_P4_TABLE_H
