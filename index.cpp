#include "ssfi.hpp"
#include <ctype.h>

const int US_PER_MS = 1000;
const int WAIT_TIME_MS = 100;

static string dequeue_file(queue<string> &file_queue)
{
	string file_path = "";

	while (file_path.empty()) {
		gbl.file_queue_mutex.lock();

		/* critical section — global file queue */
		log_debug("queue size " + std::to_string(file_queue.size()));
		if (file_queue.size() > 0) {
			file_path = file_queue.front();
			file_queue.pop();
			log_debug("dequeued file: " + file_path);

			gbl.file_queue_mutex.unlock();
		} else {
			gbl.file_queue_mutex.unlock();

			if (gbl.search_complete) {
				return file_path;
			}
			/* allow the search thread some time to populate the queue before checking again */
			log_debug("waiting...");
			usleep(US_PER_MS * WAIT_TIME_MS);
		}
	}

	return file_path;
}

static void index_word(const string &word)
{
	gbl.index_mutex.lock();	

	/* critical section — global index (map) */
	gbl.index[word]++;

	gbl.index_mutex.unlock();
}

static void index_line(const string &line)
{
	std::sregex_iterator begin = std::sregex_iterator(line.begin(), line.end(), WORD_REGEX);
	/* the default constructor creates an EOD iterator that can be used for comparison */
	std::sregex_iterator end = std::sregex_iterator();

	for (std::sregex_iterator i = begin; i != end; i++) {
		std::smatch match = *i;
		string word = match.str();
		/* convert words to their lowercase form to achieve case-insensitivity */
		std::transform(word.begin(), word.end(), word.begin(), tolower);
		index_word(word);
	}
}

static void index_file(const string &file)
{
	log_debug("indexing " + file + "...");
	ifstream stream(file);
	string line;

	while (getline(stream, line)) {
		index_line(line);
	}

	stream.close();
	log_debug("completed indexing " + file);
}

void *index_files(void *_)
{
	string file_path = "";
	int files_indexed = 0;

	log_debug("indexing...");

	/* dequeue a file to index from the file_queue */
	file_path = dequeue_file(gbl.file_queue);
	while (!file_path.empty()) {
		/* process this file */
		index_file(file_path);
		files_indexed++;
		/* dequeue_file will write an empty string to file_path when the queue is depleted */
		file_path = dequeue_file(gbl.file_queue);
	}

	log_debug("files_indexed: " + std::to_string(files_indexed));
	
	return NULL;
}
