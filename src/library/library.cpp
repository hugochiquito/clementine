/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "core/database.h"
#include "library.h"
#include "librarymodel.h"
#include "librarybackend.h"

const char* Library::kSongsTable = "songs";
const char* Library::kDirsTable = "directories";
const char* Library::kSubdirsTable = "subdirectories";
const char* Library::kFtsTable = "songs_fts";

Library::Library(BackgroundThread<Database>* db_thread, TaskManager* task_manager,
                 QObject *parent)
  : QObject(parent),
    task_manager_(task_manager),
    backend_(NULL),
    model_(NULL),
    watcher_factory_(new BackgroundThreadFactoryImplementation<LibraryWatcher, LibraryWatcher>),
    watcher_(NULL)
{
  backend_ = db_thread->CreateInThread<LibraryBackend>();
  backend_->Init(db_thread->Worker(), kSongsTable, kDirsTable, kSubdirsTable, kFtsTable);

  model_ = new LibraryModel(backend_, this);
}

void Library::set_watcher_factory(BackgroundThreadFactory<LibraryWatcher>* factory) {
  watcher_factory_.reset(factory);
}

void Library::Init() {
  watcher_ = watcher_factory_->GetThread(this);
  connect(watcher_, SIGNAL(Initialised()), SLOT(WatcherInitialised()));
}

void Library::StartThreads() {
  Q_ASSERT(watcher_);

  watcher_->set_io_priority(BackgroundThreadBase::IOPRIO_CLASS_IDLE);
  watcher_->set_cpu_priority(QThread::IdlePriority);
  watcher_->Start();

  model_->Init();
}

void Library::WatcherInitialised() {
  LibraryWatcher* watcher = watcher_->Worker().get();

  watcher->set_backend(backend_);
  watcher->set_task_manager(task_manager_);

  connect(backend_, SIGNAL(DirectoryDiscovered(Directory,SubdirectoryList)),
          watcher,  SLOT(AddDirectory(Directory,SubdirectoryList)));
  connect(backend_, SIGNAL(DirectoryDeleted(Directory)),
          watcher,  SLOT(RemoveDirectory(Directory)));
  connect(watcher,  SIGNAL(NewOrUpdatedSongs(SongList)),
          backend_, SLOT(AddOrUpdateSongs(SongList)));
  connect(watcher,  SIGNAL(SongsMTimeUpdated(SongList)),
          backend_, SLOT(UpdateMTimesOnly(SongList)));
  connect(watcher,  SIGNAL(SongsDeleted(SongList)),
          backend_, SLOT(DeleteSongs(SongList)));
  connect(watcher,  SIGNAL(SubdirsDiscovered(SubdirectoryList)),
          backend_, SLOT(AddOrUpdateSubdirs(SubdirectoryList)));
  connect(watcher,  SIGNAL(SubdirsMTimeUpdated(SubdirectoryList)),
          backend_, SLOT(AddOrUpdateSubdirs(SubdirectoryList)));
  connect(watcher, SIGNAL(CompilationsNeedUpdating()),
          backend_, SLOT(UpdateCompilations()));

  // This will start the watcher checking for updates
  backend_->LoadDirectoriesAsync();
}

void Library::IncrementalScan() {
  if (!watcher_->Worker())
    return;

  watcher_->Worker()->IncrementalScanAsync();
}

void Library::PauseWatcher() {
  if (!watcher_->Worker())
    return;

  watcher_->Worker()->SetRescanPausedAsync(true);
}

void Library::ResumeWatcher() {
  if (!watcher_->Worker())
    return;

  watcher_->Worker()->SetRescanPausedAsync(false);
}
