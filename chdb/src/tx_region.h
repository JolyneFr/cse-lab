#include "ch_db.h"


/*
 * tx_region: chdb KV client which supports transaction concurrency control.
 * */
class tx_region {
public:
    tx_region(chdb *db) : db(db),
                          tx_id(db->next_tx_id()) {
        this->tx_begin();
    }

    ~tx_region() {
        if (this->tx_can_commit() == chdb_protocol::prepare_ok) this->tx_commit();
        else this->tx_abort();
    }

    /**
     * Dummy request. Only for test
     * */
    int dummy() {
        int r;
        this->db->vserver->execute(1,
                                   chdb_protocol::Dummy,
                                   chdb_protocol::operation_var{.tx_id = tx_id, .key = 1024, .value = 16},
                                   r);
        return r;
    }

    /**
     * Put one kv into the storage
     *
     * Note!: The changes in a the transaction region cannot be viewed by other regions until it commits.
     * Meanwhile, those changes can be **seen** in self region.
     * */
    int put(const int key, const int val);

    /**
     * Query one value from the storage by `key`
     * */
    int get(const int key);

    /**
     * Transaction check whether could commit.
     * Return 1 if all of the shards are ok to commit, and 0 if exists one not ok.
     * */
    int tx_can_commit();

    /* save the result of put_cmd */
    void push_back_res(std::shared_ptr<chdb_command::result> &res);

    /*******************************************
     * Transaction part
     * ********************************************/
private:
    /**
     * Transaction begin
     * */
    int tx_begin();

    /**
     * Transaction commit. Sending `Commit` messages to all of the shard clients
    * */
    int tx_commit();

    /**
     * Transaction abort. Sending `Rollback` messages to all of the shard clients
     * */
    int tx_abort();

    int tx_prepare();

    

    chdb *db;
    const int tx_id;

private:
    std::map<int, int> put_cache;
    std::vector<std::shared_ptr<chdb_command::result>> put_result;
    int cmd_count = 0;
    void wait_all_done();
};
