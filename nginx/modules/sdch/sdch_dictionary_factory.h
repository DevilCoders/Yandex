// Author: Vasily Chekalkin <bacek@yandex-team.ru>

#ifndef SDCH_DICTIONARY_FACTORY_H_
#define SDCH_DICTIONARY_FACTORY_H_

extern "C" {
#include <ngx_config.h>
#include <nginx.h>
#include <ngx_core.h>
}

#include <vector>

#include "sdch_dictionary.h"
#include "sdch_dict_config.h"
#include "sdch_pool_alloc.h"

namespace sdch {

class DictionaryFactory {
 public:
  explicit DictionaryFactory(ngx_pool_t* pool);

  // Allocate new Dictionary. We keep ownership
  Dictionary* load_dictionary(const char* filename);

  // Allocate new DictConfig and store it internally. We keep ownership.
  DictConfig* store_config(Dictionary* dict,
                           ngx_str_t& groupname,
                           ngx_uint_t prio);

  // Search for Dictionary
  DictConfig* find_dictionary(const u_char* client_id);

  // Select best dictionary from 2.
  DictConfig* choose_best_dictionary(DictConfig* old,
                                     DictConfig* n,
                                     const ngx_str_t& group);

  // Merge data with possible parent's
  void merge(const DictionaryFactory* parent);

 private:
  typedef std::vector<Dictionary*, PoolAllocator<Dictionary*> > DictStorage;
  typedef std::vector<DictConfig, PoolAllocator<DictConfig> >  DictConfStorage;

  ngx_pool_t* pool_;
  DictStorage dict_storage_;
  DictConfStorage conf_storage_;
};


}  // namespace sdch

#endif  // SDCH_DICTIONARY_FACTORY_H_

