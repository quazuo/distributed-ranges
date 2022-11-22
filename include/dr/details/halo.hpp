namespace lib {

template <typename T> class index_group {
public:
  using element_type = T;
  std::size_t buffer_index;
  std::size_t request_index;
  bool receive;

  index_group(T *data, std::size_t rank,
              const std::vector<std::size_t> &indices)
      : data_(data), rank_(rank), indices_(indices) {}

  void unpack(T *buffer, const auto &op) {
    for (auto i : indices_) {
      drlog.debug("unpack before {}, {}: {}\n", i, data_[i], *buffer);
      data_[i] = op(data_[i], *buffer++);
      drlog.debug("       after {}\n", data_[i]);
    }
  }

  void pack(T *buffer) {
    for (auto i : indices_) {
      drlog.debug("pack {}, {}\n", i, data_[i]);
      *buffer++ = data_[i];
    }
  }
  std::size_t buffer_size() { return indices_.size(); }

  std::size_t rank() { return rank_; }

private:
  T *data_;
  std::size_t rank_;
  std::vector<std::size_t> indices_;
};

template <typename Group> class halo {
  using T = typename Group::element_type;

public:
  using group_type = Group;
  halo(communicator comm, const std::vector<Group> &owned_groups,
       const std::vector<Group> &halo_groups)
      : comm_(comm), halo_groups_(halo_groups), owned_groups_(owned_groups) {
    std::size_t buffer_size = 0;
    std::size_t i = 0;
    for (auto &g : owned_groups_) {
      g.buffer_index = buffer_size;
      g.request_index = i++;
      buffer_size += g.buffer_size();
      map_.push_back(&g);
    }
    for (auto &g : halo_groups_) {
      g.buffer_index = buffer_size;
      g.request_index = i++;
      buffer_size += g.buffer_size();
      map_.push_back(&g);
    }
    buffer_.resize(buffer_size);
    requests_.resize(i);
  }

  /// Begin a halo exchange
  void exchange_begin() {
    receive(halo_groups_);
    send(owned_groups_);
  }

  /// Complete a halo exchange
  void exchange_finalize() { reduce_finalize(second); }

  /// Begin a halo reduction
  void reduce_begin() {
    receive(owned_groups_);
    send(halo_groups_);
  }

  /// Complete a halo reduction
  void reduce_finalize(const auto &op) {
    for (int pending = requests_.size(); pending > 0; pending--) {
      int completed;
      MPI_Waitany(requests_.size(), requests_.data(), &completed,
                  MPI_STATUS_IGNORE);
      drlog.debug("Completed: {}\n", completed);
      auto &g = *map_[completed];
      if (g.receive) {
        g.unpack(&buffer_[g.buffer_index], op);
      }
    }
  }

  struct second_op {
    T operator()(T &a, T &b) const { return b; }
  } second;

  struct plus_op {
    T operator()(T &a, T &b) const { return a + b; }
  } plus;

  struct max_op {
    T operator()(T &a, T &b) const { return std::max(a, b); }
  } max;

  struct min_op {
    T operator()(T &a, T &b) const { return std::min(a, b); }
  } min;

  struct multipllies_op {
    T operator()(T &a, T &b) const { return a * b; }
  } multiplies;

private:
  void send(std::vector<Group> &sends) {
    for (auto &g : sends) {
      auto b = &buffer_[g.buffer_index];
      g.pack(b);
      g.receive = false;
      drlog.debug("Sending: {}\n", g.request_index);
      comm_.isend(b, g.buffer_size() * sizeof(T), g.rank(),
                  &requests_[g.request_index]);
    }
  }

  void receive(std::vector<Group> &receives) {
    for (auto &g : receives) {
      g.receive = true;
      drlog.debug("Receiving: {}\n", g.request_index);
      comm_.irecv(&buffer_[g.buffer_index], g.buffer_size() * sizeof(T),
                  g.rank(), &requests_[g.request_index]);
    }
  }

  communicator comm_;
  std::vector<Group> halo_groups_, owned_groups_;
  std::vector<T> buffer_;
  std::vector<MPI_Request> requests_;
  std::vector<Group *> map_;
};

template <typename T> using unstructured_halo = halo<index_group<T>>;

} // namespace lib
