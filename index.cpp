#include "ssfi.hpp"

static string dequeue_file(queue<string> &file_queue)
{
	string file_path = "";

	while (file_path.empty() && !gbl.search_complete) {
		gbl.file_queue_mutex.lock();

		/* critical section — global file queue */
		if (file_queue.size() > 0) {
			file_path = file_queue.front();
			file_queue.pop();
			log_debug("dequeued file: " + file_path);

			gbl.file_queue_mutex.unlock();
		} else {
			gbl.file_queue_mutex.unlock();
			/* allow the search thread some time to populate the queue before checking again */
			log_debug("waiting...");
			usleep(100 * 5000);
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
	while (!gbl.search_complete);

	log_debug("indexing...");

	/* dequeue a file to index from the file_queue */
	file_path = dequeue_file(gbl.file_queue);
	while (!file_path.empty()) {
		/* process this file */
		index_file(file_path);
		/* dequeue_file will write an empty string to file_path when the queue is depleted */
		file_path = dequeue_file(gbl.file_queue);
	}
	
	return NULL;
}
