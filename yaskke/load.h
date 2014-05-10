void load_map(const char *file, struct map_t *mp)
{
	int num;
	char buf[BUFSIZE], key[BUFSIZE], hira[BUFSIZE], kata[BUFSIZE];
	FILE *fp;

	fp = efopen(file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0)
			continue;

		num = sscanf(buf, "%[^\t]\t%s\t%s", key, hira, kata);
		if (num != 3)
			continue;

		if (DEBUG)
			fprintf(stderr, "key:%s hira:%s kata:%s\n", key, hira, kata);
		mp->triplets = (struct triplet_t *) realloc(mp->triplets, sizeof(struct triplet_t) * (mp->count + 1));
		mp->triplets[mp->count].key = (char *) emalloc(strlen(key) + 1);
		mp->triplets[mp->count].hira = (char *) emalloc(strlen(hira) + 1);
		mp->triplets[mp->count].kata = (char *) emalloc(strlen(kata) + 1);
		memcpy(mp->triplets[mp->count].key, key, strlen(key));
		memcpy(mp->triplets[mp->count].hira, hira, strlen(hira));
		memcpy(mp->triplets[mp->count].kata, kata, strlen(kata));
		mp->count++;
	}
	if (DEBUG)
		fprintf(stderr, "map_count:%d\n", mp->count);

	fclose(fp);
}

void load_dict(const char *file, struct table_t *okuri_ari, struct table_t *okuri_nasi)
{
	char buf[BUFSIZE], key[BUFSIZE], entry[BUFSIZE];
	int  mode = RESET, num;
	long offset;
	FILE *fp;
	struct table_t *tp;

	enum {
		READ_OKURIARI = 1,
		READ_OKURINASI,
	};

	fp = efopen(file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strstr(buf, ";; okuri-ari entries") != NULL) {
			mode = READ_OKURIARI;
			continue;
		}
		else if (strstr(buf, ";; okuri-nasi entries") != NULL) {
			mode = READ_OKURINASI;
			continue;
		}

		if (mode == RESET)
			continue;

		num = sscanf(buf, "%s %s", key, entry);
		if (num != 2)
			continue;

		offset = ftell(fp) - strlen(buf);

		if (DEBUG)
			fprintf(stderr, "key:%s entry:%s offset:%ld\n", key, entry, offset);
		tp = (mode == READ_OKURIARI) ? okuri_ari: okuri_nasi;

		tp->entries = (struct entry_t *) realloc(tp->entries, sizeof(struct entry_t) * (tp->count + 1));
		tp->entries[tp->count].key = (char *) emalloc(strlen(key) + 1);

		strcpy(tp->entries[tp->count].key, key);
		tp->entries[tp->count].offset = offset;
		tp->count++;
	}
	if (DEBUG)
		fprintf(stderr, "okuri_ari:%d okuri_nasi:%d\n", okuri_ari->count, okuri_nasi->count);

	fclose(fp);
}

void load_user(const char *file, struct hash_t *user_dict[])
{
	int i, num;
	char buf[BUFSIZE], key[BUFSIZE], entry[BUFSIZE];
	FILE *fp;
	struct parm_t parm;

	fp = efopen(file, "a+");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		num = sscanf(buf, "%s %s", key, entry);
		if (num != 2)
			continue;

		if (DEBUG)
			fprintf(stderr, "key:%s entry:%s\n", key, entry);
		
		reset_parm(&parm);
		parse_entry(entry, &parm, '/', not_slash);

		for (i = 0; i < parm.argc; i++) {
			if (DEBUG)
				fprintf(stderr, "argv[%d]: %s\n", i, parm.argv[i]);
			hash_create(user_dict, key, parm.argv[i]);
		}
	}

	fclose(fp);
}
