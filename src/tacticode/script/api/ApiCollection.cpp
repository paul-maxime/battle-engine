#include "api_intern.hpp"

#include "tacticode/script/ScriptFactory.hpp"
#include "tacticode/utils/Singleton.hpp"
#include "tacticode/script/script_intern.hpp"
#include "tacticode/system/Vector2.hpp"

#include <sstream>
#include <iostream>
#include <cassert>

#include "tacticode/engine/BattleEngine.hpp"
#include "tacticode/engine/Character.hpp"
#include "tacticode/utils/FightLogger.hpp"

#include "injectSpellApi.hpp"

using tacticode::utils::Singleton;
using tacticode::script::ScriptFactory;
using tacticode::script::v8String;
using tacticode::script::Context;

namespace {
	using namespace tacticode;

  void functionLog(const v8::FunctionCallbackInfo<v8::Value>& args) {
  	std::ostringstream stringStream;
			    stringStream << "{\"type\":\"console\", \"data\":\"";
			    if (args.Length() >= 1) {
			    v8::HandleScope scope(args.GetIsolate());
			    v8::Local<v8::Value> arg = args[0];
			    v8::String::Utf8Value value(arg);

			    stringStream << *value;
			    }
			    stringStream << "\"}";
			    std::cerr << stringStream.str();
  }

  void functionFireBall(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() >= 2) {
    v8::HandleScope scope(args.GetIsolate());
    v8::Local<v8::Value> argX = args[0];
    v8::Local<v8::Value> argY = args[1];

    (void)argX;
    (void)argY;
    //Todo, fireball somehow
    }
  }

  tacticode::engine::BattleEngineContext* getBattleContext(v8::Local<v8::Context>& context) {
	  auto battle = reinterpret_cast<tacticode::engine::BattleEngineContext*>(
		  context->GetAlignedPointerFromEmbedderData(Context::BATTLE_ENGINE));
	  assert(battle);
	  assert(battle->character);
	  return battle;
  }

  v8::Local<v8::Object> createV8Entity(v8::Isolate* isolate, engine::Character& character) {
	  v8::Local<v8::Object> result = v8::Object::New(isolate);
	  v8::Local<v8::Context> context = isolate->GetCurrentContext();

	  result->Set(context, v8String::fromString("id"), v8::Number::New(isolate, character.getId()));
	  result->Set(context, v8String::fromString("name"), v8String::fromString(character.getName()));
	  result->Set(context, v8String::fromString("x"), v8::Number::New(isolate, character.getPosition().x));
	  result->Set(context, v8String::fromString("y"), v8::Number::New(isolate, character.getPosition().x));
	  result->Set(context, v8String::fromString("team"), v8::Number::New(isolate, character.getTeamId()));
	  result->Set(context, v8String::fromString("race"), v8String::fromString(character.getBreedString()));
	  result->Set(context, v8String::fromString("health"), v8::Number::New(isolate, character.getCurrentHealth()));
	  result->Set(context, v8String::fromString("maxHealth"), v8::Number::New(isolate, character.getBaseAttributes().health));
	  result->Set(context, v8String::fromString("movement"), v8::Number::New(isolate, character.getCurrentMovementPoints()));
	  result->Set(context, v8String::fromString("maxMovement"), v8::Number::New(isolate, character.getBaseAttributes().movement));

	  // TODO skills array

	  return result;
  }

  void functionGetCurrentEntity(const v8::FunctionCallbackInfo<v8::Value>& args) {
	  v8::EscapableHandleScope scope(args.GetIsolate());
	  v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	  auto battle_context = getBattleContext(context);
	  auto character = battle_context->character;

	  v8::Local<v8::Object> entity = createV8Entity(args.GetIsolate(), *character);

	  args.GetReturnValue().Set(scope.Escape(entity));
  }

  void functionGetEntities(const v8::FunctionCallbackInfo<v8::Value>& args) {
	  v8::EscapableHandleScope scope(args.GetIsolate());
	  v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	  auto battle_context = getBattleContext(context);
	  auto& characters = battle_context->engine->getCharacters();

	  v8::Local<v8::Array> list = v8::Array::New(args.GetIsolate(), characters.size());
	  for (size_t i = 0; i < characters.size(); ++i) {
		  list->Set(i, createV8Entity(args.GetIsolate(), *characters[i]));
	  }

	  args.GetReturnValue().Set(scope.Escape(list));
  }

  void functionGetCurrentEntityOnCell(const v8::FunctionCallbackInfo<v8::Value>& args) {
	  v8::EscapableHandleScope scope(args.GetIsolate());
	  v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();

	  auto battle_context = getBattleContext(context);
	  auto engine = battle_context->engine;

	  if (args.Length() >= 2) {
		  v8::Local<v8::Value> argX = args[0];
		  v8::Local<v8::Value> argY = args[1];

		  if (argX->IsNumber() && argY->IsNumber()) {
			  int32_t x = static_cast<int32_t>(argX->ToNumber()->Value());
			  int32_t y = static_cast<int32_t>(argY->ToNumber()->Value());

			  auto character = engine->getCharacterOnCell(x, y);
			  if (character) {
				  v8::Local<v8::Object> entity = createV8Entity(args.GetIsolate(), *character);
				  args.GetReturnValue().Set(scope.Escape(entity));
			  }
		  }
	  }
	  args.GetReturnValue().Set(scope.Escape(v8::Null(args.GetIsolate())));
  }

  void functionMoveToCell(const v8::FunctionCallbackInfo<v8::Value>& args) {
  	v8::EscapableHandleScope scope(args.GetIsolate());
  	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
  	bool rt = false;
  	//BattleEngineContext stored in global of the context
  	tacticode::engine::BattleEngineContext *battle_context = reinterpret_cast<tacticode::engine::BattleEngineContext*>(
  		context->GetAlignedPointerFromEmbedderData(Context::BATTLE_ENGINE));

  	assert(battle_context);

  	auto character = battle_context->character;

  	assert(character);
    if (args.Length() >= 2) {
	    v8::Local<v8::Value> argX = args[0];
	    v8::Local<v8::Value> argY = args[1];
	    if (argX->IsNumber() && argY->IsNumber()) {
 		    int32_t x = static_cast<int32_t>(argX->ToNumber()->Value());
			int32_t y = static_cast<int32_t>(argY->ToNumber()->Value());
		    rt = character->moveToCell(engine::Vector2i(x, y));

		    auto action = utils::Log::Action(character->getId(), x, y, "move");
		    action.add("success", rt);
		    utils::Singleton<utils::FightLogger>::GetInstance()->addAction(action);
	    }
    }
  	v8::Local<v8::Value> result = v8::Boolean::New(args.GetIsolate(), rt);
  	args.GetReturnValue().Set(scope.Escape(result));
  }

}

namespace tacticode{
namespace script{
namespace api{

ApiCollection::ApiCollection() {
}

void ApiCollection::injectApi(std::shared_ptr<tacticode::script::Context> context) {
	v8::Isolate *isolate = Singleton<ScriptFactory>::GetInstance()->getEngine();

	auto global = context->get()->Global();
	{
		//Inject a function like this
		tacticode::script::v8String funcName("$log");
		global->Set(funcName.get(), v8::Function::New(isolate, functionLog));		
	}

	{
		tacticode::script::v8String funcName("$fireball");
		global->Set(funcName.get(), v8::Function::New(isolate, functionFireBall));
	}	
	global->Set(v8String::fromString("getCurrentEntity"), v8::Function::New(isolate, functionGetCurrentEntity));
	global->Set(v8String::fromString("moveToCell"), v8::Function::New(isolate, functionMoveToCell));

  injectSpellApi(context);
}

ApiCollection::~ApiCollection() {
}
}//api
}//script
}//tacticode
