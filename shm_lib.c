// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "shm_lib.h"

#define SEM_NAME_DATA_AVAILABLE "/data_available_semaphore"
#define ERROR 1
#define SHM_OPEN_MODE 0666
#define SEM_OPEN_MODE 0644

typedef struct shared_memory_cdt {
	char * name;
	char * start;
	int fd;
	size_t size;
	int is_writer;
	size_t offset;
	sem_t *data_available;
} shared_memory_cdt;



shared_memory_adt get_shm ( const char *name, bool is_creator, bool is_writer )
{
	shared_memory_adt shm = malloc ( sizeof ( shared_memory_cdt ) );

	if ( shm == NULL ) {
		perror ( "Error: Memory allocation failed for shared_memory_cdt" );
		return NULL;
	}

	shm->name = strdup ( name );

	if ( shm->name == NULL ) {
		perror ( "Error: Failed to duplicate the name string for shared memory" );
		free ( shm );
		return NULL;
	}

	shm->is_writer = is_writer;

	if ( is_creator ) {
		unlink_shm ( shm ); // Performing this unlink might return an error, but it's the only way to ensure that another process didn't leave a shared memory segment and semaphores open.
	}

	int oflag = is_creator ? ( O_RDWR | O_CREAT ) : ( O_RDWR );
	shm->fd = shm_open ( shm->name, oflag, SHM_OPEN_MODE );

	if ( shm->fd == -1 ) {
		perror ( "Error: Failed to open shared memory" );
		free ( shm );
		return NULL;
	}


	if ( is_creator ) {
		if ( ftruncate ( shm->fd, SHM_SIZE ) == -1 ) {
			perror ( "Error: Failed to set shared memory size" );
			shm_unlink ( shm->name );
			close ( shm->fd );
			free ( shm->name );
			free ( shm );
			return NULL;
		}
	}

	shm->start = mmap ( 0, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm->fd, 0 );

	if ( shm->start == MAP_FAILED ) {
		perror ( "Error: Memory mapping failed" );
		shm_unlink ( shm->name );
		close ( shm->fd );
		free ( shm->name );
		free ( shm );
		return NULL;
	}

	shm->data_available = sem_open ( SEM_NAME_DATA_AVAILABLE, O_CREAT, SEM_OPEN_MODE, 0 );

	if  ( shm->data_available == SEM_FAILED ) {
		perror ( "Error: Failed to open data availability semaphore" );
		shm_unlink ( shm->name );
		munmap ( shm->start, SHM_SIZE );
		close ( shm->fd );
		free ( shm->name );
		free ( shm );
		return NULL;
	}

	shm->size = SHM_SIZE;
	shm->offset = 0;
	return shm;
}

ssize_t write_shm ( const char *buffer, shared_memory_adt segment, size_t buffer_size )
{

	if ( segment == NULL || buffer == NULL || ! ( segment->is_writer ) ) {
		return 0;
	}

	size_t bytes_written = 0;

	while ( bytes_written <= buffer_size && segment->offset < SHM_SIZE ) {
		char byte = buffer[bytes_written++];

		segment->start[segment->offset++] = byte;

		if ( byte == END_OF_READ ) {
			break;
		}
	}

	if ( ! ( segment->offset < SHM_SIZE ) ) {
		perror ( "No more memory in shared memory" );
		return -1;
	}

	sem_post ( segment->data_available );
	return bytes_written;
}

ssize_t read_shm ( char *buffer, shared_memory_adt segment, size_t buffer_size )
{

	if ( segment == NULL || buffer == NULL || ( segment->is_writer ) ) {
		return -1;
	}

	size_t bytes_read = 0;

	sem_wait ( segment->data_available );

	while ( bytes_read < buffer_size && segment->offset < SHM_SIZE ) {
		char byte = segment->start[segment->offset++];
		buffer[bytes_read] = byte;

		if ( byte == END_OF_READ ) {
			buffer[bytes_read++] = 0;
			break;
		}

		bytes_read++;
	}

	if ( ! ( segment->offset < SHM_SIZE ) ) {
		perror ( "No more memory in shared memory" );
		return -1;
	}

	return bytes_read;
}

void unlink_shm ( shared_memory_adt segment )
{
	sem_unlink ( SEM_NAME_DATA_AVAILABLE );
	shm_unlink ( segment->name );
}


void close_shm ( shared_memory_adt segment )
{
	if ( sem_close ( segment->data_available ) == -1 ) {
		perror ( "Error: Failed to close data_available semaphore" );
	}

	if ( munmap ( segment->start, SHM_SIZE ) == -1 ) {
		perror ( "Error: Failed to unmap shared memory region" );
	}

	if ( close ( segment->fd ) == -1 ) {
		perror ( "Error: Failed to close shared memory file descriptor" );
	}

	free ( segment->name );
	free ( segment );
	segment = NULL;
}