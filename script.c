/*
 * Copyright (C) 2012  Alejandro Mery <amery@geeks.cl>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sunxi-tools.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "script.h"

/*
 */
struct script *script_new(void)
{
	struct script *script;
	if ((script = malloc(sizeof(*script))))
		list_init(&script->sections);
	return script;
}

void script_delete(struct script *script)
{
	struct list_entry *o;

	assert(script);

	while ((o = list_last(&script->sections))) {
		struct script_section *section = container_of(o,
			       struct script_section, sections);

		script_section_delete(section);
	}

	free(script);
}

/*
 */
struct script_section *script_section_new(struct script *script,
					  const char *name)
{
	struct script_section *section;

	assert(script);
	assert(name && *name);

	if ((section = malloc(sizeof(*section)))) {
		size_t l = strlen(name);
		if (l>31) /* truncate */
			l=31;
		memcpy(section->name, name, l);
		section->name[l] = '\0';

		list_init(&section->entries);
		list_append(&script->sections, &section->sections);
	}
	return section;
}

void script_section_delete(struct script_section *section)
{
	struct list_entry *o;

	assert(section);

	while ((o = list_last(&section->entries))) {
		struct script_entry *entry = container_of(o,
			       struct script_entry, entries);

		script_entry_delete(entry);
	}

	if (!list_empty(&section->sections))
		list_remove(&section->sections);
}

/*
 */
static inline void script_entry_append(struct script_section *section,
				       struct script_entry *entry,
				       enum script_value_type type,
				       const char *name)
{
	size_t l;

	assert(section);
	assert(entry);
	assert(name);

	l = strlen(name);
	if (l>31) /* truncate */
		l=31;
	memcpy(entry->name, name, l);
	entry->name[l] = '\0';

	entry->type = type;

	list_append(&section->entries, &entry->entries);
}

void script_entry_delete(struct script_entry *entry)
{
	void *container;

	assert(entry);
	assert(entry->type == SCRIPT_VALUE_TYPE_SINGLE_WORD ||
	       entry->type == SCRIPT_VALUE_TYPE_STRING ||
	       entry->type == SCRIPT_VALUE_TYPE_GPIO ||
	       entry->type == SCRIPT_VALUE_TYPE_NULL);

	if (!list_empty(&entry->entries))
		list_remove(&entry->entries);

	switch(entry->type) {
	case SCRIPT_VALUE_TYPE_SINGLE_WORD:
		container = container_of(entry, struct script_single_entry, entry);
		break;
	case SCRIPT_VALUE_TYPE_STRING:
		container = container_of(entry, struct script_string_entry, entry);
		break;
	case SCRIPT_VALUE_TYPE_GPIO:
		container = container_of(entry, struct script_gpio_entry, entry);
		break;
	case SCRIPT_VALUE_TYPE_NULL:
		container = container_of(entry, struct script_null_entry, entry);
		break;
	default:
		abort();
	}

	free(container);
}

struct script_null_entry *script_null_entry_new(struct script_section *section,
						const char *name)
{
	struct script_null_entry *entry;

	assert(section);
	assert(name && *name);

	if ((entry = malloc(sizeof(*entry)))) {
		script_entry_append(section, &entry->entry,
				    SCRIPT_VALUE_TYPE_NULL, name);
	}

	return entry;
}

struct script_single_entry *script_single_entry_new(struct script_section *section,
						    const char *name,
						    uint32_t value)
{
	struct script_single_entry *entry;

	assert(section);
	assert(name && *name);

	if ((entry = malloc(sizeof(*entry)))) {
		entry->value = value;

		script_entry_append(section, &entry->entry,
				    SCRIPT_VALUE_TYPE_SINGLE_WORD, name);
	}

	return entry;
}

struct script_string_entry *script_string_entry_new(struct script_section *section,
						       const char *name,
						       size_t l, const char *s)
{
	struct script_string_entry *entry;

	assert(section);
	assert(name);
	assert(s);

	if ((entry = malloc(sizeof(*entry)+l+1))) {
		entry->l = l;
		memcpy(entry->string, s, l);
		entry->string[l] = '\0';

		script_entry_append(section, &entry->entry,
				    SCRIPT_VALUE_TYPE_STRING, name);
	}

	return entry;
}

struct script_gpio_entry *script_gpio_entry_new(struct script_section *section,
						const char *name,
						unsigned port, unsigned num,
						int32_t data[4])
{
	struct script_gpio_entry *entry;

	assert(section);
	assert(name && *name);

	if ((entry = malloc(sizeof(*entry)))) {
		entry->port = port;
		entry->port_num = num;
		for (int i=0; i<4; i++)
			entry->data[i] = data[i];

		script_entry_append(section, &entry->entry,
				    SCRIPT_VALUE_TYPE_GPIO, name);
	}

	return entry;
}
