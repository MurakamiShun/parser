#pragma once
#include "tokenizer.hpp"
#include <functional>
#include <utility>
#include <memory>

constexpr size_t MATCHING_DEPTH_MAX = 100;

template<typename Iter>
struct span {
	Iter begin;
	Iter end;
};

template<typename First, typename Second>
struct radio {
	std::optional<First> first;
	std::optional<Second> second;
	radio(const First& arg) :first(arg) {
		second = std::nullopt;
	}
	radio(const Second& arg) :second(arg) {
		first = std::nullopt;
	}
};


enum class NodeID;

struct Node {
	NodeID id;

	std::vector<std::unique_ptr<Node>> child;

	std::vector<Token>::iterator begin;
	std::vector<Token>::iterator end;

	using token_iterator = std::vector<Token>::iterator;
	using detector_func_impl = std::function<std::unique_ptr<Node>(span<token_iterator>&)>;
	using detector_func = std::shared_ptr<detector_func_impl>;
	std::vector<detector_func> detectors;
	
	Node(const NodeID _id) { id = _id; }
	Node(const Node& node) {
		id = node.id;
		begin = node.begin;
		end = node.end;
		detectors = node.detectors;
	}

	void clear_detectors() {
		detectors.clear();
		for (auto& c : child) c->detectors.clear();
	}

	Node& has(detector_func func) {
		detectors.push_back(func);
		return *this;
	}
	Node& is(detector_func func) {
		detectors.push_back(func);
		return *this;
	}

	void parse() {
		auto iter = begin;
		if (detectors.size() && child.size() == 0) {
			while (iter != end) {
				bool hit = false;
				for (auto& detector : detectors) {
					span<token_iterator> it = { iter, end };
					auto node = (*detector)(it);
					if (node) {
						iter = it.begin;
						hit = true;
						child.push_back(std::move(node));
						break;
					}
				}
				if (!hit) {
					std::cout << "ƒGƒ‰[" << (int)id << std::endl;
					for (auto i = iter; i != end; ++i) {
						std::cout << i->text << " ";
					}
					std::cout << "\n\n";
					// raise error
					iter++;
					break;
				}
			}
		}
		
		for (auto& c : child) { c->parse(); }
	}

	std::string fmt(size_t indent = 0) {
		std::string tags;
		std::string rtn;
		for (auto i = 0; i < indent; ++i) { tags += "|"; }
		rtn = tags + "-" + std::to_string((int)id) + " : ";
		for (auto iter = begin; iter != end; ++iter) {
			rtn += iter->text;
			rtn += " ";
		}
		rtn+="\n";
		//for (auto iter = begin; iter != end; iter++) rtn += iter->text;
		for (auto& c : child) { rtn += c->fmt(indent + 1); }
		return rtn;
	}

	struct Rule {
		static detector_func paren(const TokenID open, const Node& sample, const TokenID close) {
			return std::make_shared<detector_func_impl>([=, &sample](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }
				if (target.begin->id != open) { return nullptr; }
				
				auto node = std::make_unique<Node>(sample);
				node->begin = target.begin + 1;
				int hierarchy = 1;
				while (hierarchy != 0) {
					++target.begin;
					if (target.begin == target.end) { return nullptr; }
					if (target.begin->id == open) ++hierarchy;
					else if (target.begin->id == close) --hierarchy;
				}
				node->end = target.begin;
				target.begin++;
				return std::move(node);
			});
		}

		template<typename... Args>
		static detector_func match(const Node& sample, const Args&... args) {
			
			return std::make_shared<detector_func_impl>([&sample, &args...](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }
				std::vector<radio<TokenID, Node>> matchers = { args... };

				int64_t untreate = (int64_t)matchers.size();
				if (std::distance(target.begin, target.end) < untreate) { return nullptr; }

				auto node = std::make_unique<Node>(sample);
				node->begin = target.begin;

				for (auto& matcher : matchers) {
					--untreate;
					if (target.begin == target.end) { return nullptr; }
					if (matcher.first) {
						if (target.begin->id != matcher.first.value()) { return nullptr; }
						++target.begin;
					}
					else {
						std::unique_ptr<Node> child_node;
						span<token_iterator> it = { target.begin, target.end - untreate };
						for (auto& detector : matcher.second->detectors) {
							child_node = std::move((*detector)(it));
							if (child_node) { break; }
						}
						if (child_node) {
							target.begin = it.begin;
							node->child.push_back(std::move(child_node));
						}
						else { return nullptr; }
					}
				}
				node->end = target.begin;
				return std::move(node);
			});
		}

		static detector_func binary_operator(const Node& sample,const Node& left, const TokenID token_id, const Node& right) {
			return std::make_shared<detector_func_impl>([&sample, &left, token_id, &right](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }
				
				if (std::distance(target.begin, target.end) < 3) { return nullptr; }

				//Œã‚ë‚Ì‚Ù‚¤‚ªŒã‚ÉŽÀs‚³‚ê‚é‚Ì‚ÅŒã‚ë‚©‚ç–Ø‚É“ü‚ê‚Ä‚¢‚­
				std::unique_ptr<Node> root_node;
				for (auto iter = target.end - 1; iter != target.begin; --iter) {
					if (token_id == iter->id) {
						root_node = std::make_unique<Node>(sample);

						std::unique_ptr<Node> left_node;
						span<token_iterator> left_iter = { target.begin, iter };
						for (auto& detector : left.detectors) {
							left_node = std::move((*detector)(left_iter));
							if (left_node) { break; }
						}
						if (left_node) { root_node->child.push_back(std::move(left_node)); }
						else { continue; }
						
						std::unique_ptr<Node> right_node;
						span<token_iterator> right_iter = { iter + 1, target.end };
						for (auto& detector : right.detectors) {
							right_node = std::move((*detector)(right_iter));
							if (right_node) { break; }
						}
						if (right_node) { root_node->child.push_back(std::move(right_node)); }
						else { continue; }
						
						root_node->begin = target.begin;
						root_node->end = target.end;
						target.begin = target.end;
						return root_node;
					}
				};
				return nullptr;
			});
		}



		template<class... Args>
		static detector_func endpoint(const Node& sample, const Args... token_ids) {
			std::vector<TokenID> tokens = { token_ids... };

			return std::make_shared<detector_func_impl>([&sample, tokens](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }

				auto node = std::make_unique<Node>(sample);
				node->begin = target.begin;
				for (auto token_id : tokens) {
					if (target.begin->id != token_id) { return nullptr; }
					target.begin++;
				}
				if (target.begin != target.end) { return nullptr; }
				else {
					node->end = target.begin;
					return std::move(node);
				}
			});
		}

		static detector_func until(const Node& sample, const TokenID token_id) {
			return std::make_shared<detector_func_impl>([=, &sample](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }

				auto node = std::make_unique<Node>(sample);
				node->begin = target.begin;
				while (target.begin->id != token_id) {
					if (target.begin == target.end) { return nullptr; }
					++target.begin;
				}
				node->end = target.begin;
				target.begin++;
				return std::move(node);
			});
		}

		static detector_func slice(const Node& sample, const TokenID token_id) {
			return std::make_shared<detector_func_impl>([=, &sample](span<token_iterator>& target)->std::unique_ptr<Node> {
				if (target.begin == target.end) { return nullptr; }

				auto node = std::make_unique<Node>(sample);
				node->begin = target.begin;
				while (target.begin->id != token_id) {
					if (target.begin == target.end) {
						node->end = target.begin;
						return std::move(node);
					}
					++target.begin;
				}
				node->end = target.begin;
				target.begin++;
				return std::move(node);
			});
		}
	};
};