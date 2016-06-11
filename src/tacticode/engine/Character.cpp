#include "Character.hpp"
#include "tacticode/effect/IEffect.hpp"
#include "tacticode/spell/ISpell.hpp"
#include "tacticode/file/error/InvalidConfiguration.hpp"
#include "tacticode/utils/utils.hpp"
#include "ICharacterScript.hpp"
#include "tacticode/script/ScriptFactory.hpp"

#include <cstring>

namespace tacticode
{
	namespace engine
	{

		const std::array<std::string, 4> Character::validBreeds =
			{ "elf", "gobelin", "human", "orc" };
		const std::array<std::string, 8> Character::validAttributes =
			{ "health", "attack", "power", "defense", "resilience", "luck", "movement", "speed" };

		// no braces initialization with make_unique
		Character::Attributes::Attributes(
			int32_t health_, int32_t attack_, int32_t power_,
			int32_t defense_, int32_t resilience_, int32_t luck_,
			int32_t movement_, int32_t speed_)
			:
			health(health_), attack(attack_), power(power_),
			defense(defense_), resilience(resilience_),
			luck(luck_), movement(movement_), speed(speed_)
		{
		}

		Character::Character(const file::IValue& json)
		{
			#ifdef V8LINK
			_script = utils::Singleton<script::ScriptFactory>::GetInstance()->newCharacterScript();
			#endif
			deserialize(json);
		}

		void Character::assertAttributeDeserialize(const file::IValue& json, std::string attribute)
		{
			if (!json.hasField(attribute))
			{
				throw file::error::InvalidConfiguration("character", "character has no " + attribute);
			}
			if (!json[attribute]->isNumeric())
			{
				throw file::error::InvalidConfiguration("character", attribute + " field is not a string");
			}
			if (json[attribute]->asInt() < 0)
			{
				throw file::error::InvalidConfiguration("character", attribute + " cannot be less than 0");
			}
		}

		void Character::deserializeAttributes(const file::IValue& json)
		{
			for (auto & e : validAttributes)
			{
				assertAttributeDeserialize(json, e);
			}
			if (json["health"]->asInt() == 0)
			{
				throw file::error::InvalidConfiguration("character", "health must be greater than 0");
			}
			m_baseAttributes = std::make_unique<Attributes>(
				static_cast<int32_t>(json["health"]->asInt()),
				static_cast<int32_t>(json["attack"]->asInt()),
				static_cast<int32_t>(json["power"]->asInt()),
				static_cast<int32_t>(json["defense"]->asInt()),
				static_cast<int32_t>(json["resilience"]->asInt()),
				static_cast<int32_t>(json["luck"]->asInt()),
				static_cast<int32_t>(json["movement"]->asInt()),
				static_cast<int32_t>(json["speed"]->asInt())
			);
			m_currentAttributes = std::make_unique<Attributes>(*m_baseAttributes);
			m_currentHealth = m_baseAttributes->health;
		}

		void Character::deserialize(const file::IValue& json)
		{
			if (!json.isObject())
			{
				throw file::error::InvalidConfiguration("character", "item of characters field is not an object");
			}
			if (!json.hasField("name"))
			{
				throw file::error::InvalidConfiguration("character", "character has no name");
			}
			if (!json["name"]->isString())
			{
				throw file::error::InvalidConfiguration("character", "name field is not a string");
			}
			m_name = json["name"]->asString();
			if (!json.hasField("breed"))
			{
				throw file::error::InvalidConfiguration("character", "character has no breed");
			}
			if (!json["breed"]->isString())
			{
				throw file::error::InvalidConfiguration("character", "breed field is not a string");
			}
			std::string breed = json["breed"]->asString();
			if (!isValidBreed(breed))
			{
				throw file::error::InvalidConfiguration("character", "breed (" + breed + ") is not a valid string");
			}
			m_breed = stringToBreed(breed);
			if (!json.hasField("spells"))
			{
				throw file::error::InvalidConfiguration("character", "character has no spells field");
			}
			if (!json["spells"]->isArray())
			{
				throw file::error::InvalidConfiguration("character", "spells field is not an array");
			}
			auto _spells = json["spells"];
			const auto & spells = *_spells;
			for (int32_t i = 0; i < spells.size(); ++i)
			{
				if (!spells[i]->isString())
				{
					throw file::error::InvalidConfiguration("character", "item of spells is not a string");
				}
				addSpell(spells[i]->asString());
			}
			deserializeAttributes(json);
			if (_script != nullptr)
			setScript(json.getString("script", std::string("$log('unset") + __FILE__ + "': )"));
		}

		void Character::addSpell(const std::string & spellName) // TODO: Wilko
		{
			// this should throw an exception if the spellName does not exist: tacticode::spell::error::InvalidSpellName()
			//std::unique_ptr<spell::ISpell> ptr = spellFactory().createSpell(spellName);
		}

		Character::Breed Character::stringToBreed(const std::string & breed)
		{
			for (int32_t i = 0; i < validBreeds.size(); ++i)
			{
				if (breed == validBreeds[i])
				{
					return static_cast<Breed>(i);
				}
			}
			throw std::runtime_error("tried to convert an invalid breed string");
		}

		bool Character::isValidBreed(const std::string & breed)
		{
			for (auto & e : validBreeds)
			{
				if (breed == e)
				{
					return true;
				}
			}
			return false;
		}

		void Character::applyEffects()
		{
			for (auto & effect : m_effects)
			{
				effect->apply(*this);
			}
		}

		void Character::play(BattleEngineContext& context)
		{
			applyEffects();
			executeScript(context);
		}

		void Character::executeScript(BattleEngineContext& context)
		{
			// TODO			
			if (_script != nullptr) {
				memcpy(_script->getBattleEngineContext(), &context, sizeof(context));
				_script->run();
			}
		}

		void Character::setScript(const std::string & script)
		{
			_script->setScript(script);
		}

		const std::string& Character::getScript() const
		{
			return m_script;
		}

		int32_t Character::getTeamId() const
		{
			return m_teamId;
		}

		void Character::addEffect(std::unique_ptr<effect::IEffect> effect)
		{
			m_effects.push_back(std::move(effect));
		}

		//todo : prevoir cas spéciaux si besoin
		void Character::applyDamage(int32_t damages)
		{
			m_currentAttributes->health -= damages;
		}
		void Character::applyHeal(int32_t heal)
		{
			m_currentAttributes->health += heal;
		}
	}
}
