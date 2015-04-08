/*
 * The GPL License
 */

#include "simulationcraft.h"

namespace simnode {

Persistent<Object> module;

static Handle<Object> CreateTypeObject() {
	NanEscapableScope();
	Handle<Object> o = NanNew<Object>();
	ImmutableSet(o, NanNew<String>("commit"), NanNew<Number>(GIT_OBJ_COMMIT));
	ImmutableSet(o, NanNew<String>("tree"), NanNew<Number>(GIT_OBJ_TREE));
	ImmutableSet(o, NanNew<String>("blob"), NanNew<Number>(GIT_OBJ_BLOB));
	ImmutableSet(o, NanNew<String>("tag"), NanNew<Number>(GIT_OBJ_TAG));
	return NanEscapeScope(o);
}

extern "C" void
init(Handle<Object> target) {
	NanScope();
  module = Persistent<Object>::New(target);

	// Initialize libgit2's thread system.
	git_threads_init();

	Signature::Init();
	Repository::Init(target);
	Commit::Init(target);
	Tree::Init(target);
	Blob::Init(target);
	Tag::Init(target);
	Index::Init(target);

	Remote::Init(target);

	ImmutableSet(target, NanNew<String>("minOidLength"), NanNew<Number>(GIT_OID_MINPREFIXLEN));
	ImmutableSet(target, NanNew<String>("types"), CreateTypeObject());

	NODE_DEFINE_CONSTANT(target, GIT_DIRECTION_PUSH);
	NODE_DEFINE_CONSTANT(target, GIT_DIRECTION_FETCH);

	/*
	IndexEntry::Init(target);

	RevWalker::Init(target);
	Reference::Init(target);

	ErrorInit(target);*/
}

Handle<Object> GetModule() {
	return module;
}

} // namespace simnode

NODE_MODULE(simnode, simnode::init)
