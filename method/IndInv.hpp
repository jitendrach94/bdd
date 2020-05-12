#ifndef IND_INV_HPP_
#define IND_INV_HPP_

#include <fstream>
#include <algorithm>
#include <chrono>
#include "NtkBdd.hpp"

void CopyAigRec(mockturtle::aig_network & aig, mockturtle::aig_network & aig2, std::map<mockturtle::aig_network::signal, mockturtle::aig_network::signal> & m, mockturtle::aig_network::node const & x) {
  if(aig.is_ci(x))
    return;
  std::vector<mockturtle::aig_network::signal> fanin;
  aig.foreach_fanin(x, [&](auto fi) {
			 CopyAigRec(aig, aig2, m, aig.get_node(fi));
			 if(aig.is_complemented(fi))
			   fanin.push_back(m[aig.create_not(fi)]);
			 else 
			   fanin.push_back(m[fi]);
		       });
  assert(fanin.size() == 2);
  m[aig.make_signal(x)] = aig2.create_and(fanin[0], fanin[1]);
}

void CombNoPo(mockturtle::aig_network & aig, mockturtle::aig_network & aig2) {
  std::map<mockturtle::aig_network::signal, mockturtle::aig_network::signal> m;
  aig.foreach_pi([&](auto pi) {
		   m[aig.make_signal(pi)] = aig2.create_pi();
		 });
  aig.foreach_register([&](auto reg) {
			 m[aig.make_signal(reg.second)] = aig2.create_pi();
		       });
  aig.foreach_register([&](auto reg) {
			 CopyAigRec(aig, aig2, m, aig.get_node(reg.first));
			 if(aig.is_complemented(reg.first))
			   aig2.create_po(m[aig.create_not(reg.first)]);
			 else
			   aig2.create_po(m[reg.first]);
		       });
}

template <typename node>
void show_bdd_rec(Bdd::BddMan<node> & bdd, node x, int npis, std::string & s, std::ofstream * f, std::string & count) {
  if(x == bdd.Const1())
    {
      if(f) {
	*f << s << std::endl;
      }
      int idx = 0;
      for(int i = 0; i < s.size(); i++) {
	if(s[i] == '-') {
	  idx++;
	}
      }
      while(count[idx] != '0') {
	count[idx] = '0';
	idx++;
      }
      count[idx] = '1';
      return;
    }
  if(x == bdd.Const0())
    return;
  s[bdd.Var(x) - npis] = '1';
  show_bdd_rec(bdd, bdd.Then(x), npis, s, f, count);
  s[bdd.Var(x) - npis] = '0';
  show_bdd_rec(bdd, bdd.Else(x), npis, s, f, count);
  s[bdd.Var(x) - npis] = '-';
}

template <typename node> 
void show_bdd(Bdd::BddMan<node> & bdd, node & x, int npis, int nregs, std::ofstream * f) {
  std::string s;
  for(int i = 0; i < nregs; i++) {
    s += "-";
  }
  std::string count;
  for(int i = 0; i < nregs + 1; i++) {
    count += "0";
  }
  show_bdd_rec(bdd, x, npis, s, f, count);
  
  std::cout << "iig size : ";
  for(int i = 0; i < nregs / 32 + 1; i++) {
    uint count_int = 0;
    for(int j = 0 ; j < 32 && j + i * 32 < count.size(); j++) {
      if(count[j + i * 32] == '1') {
	count_int += 1 << j;
      }
    }
    if(!i) {
      std::cout << count_int;
    }
    else {
      std::cout << " + " << count_int << " * 2^" << 32 * i;
    }
  }

  std::cout << " / ";
  std::cout << ((uint)1 << (nregs % 32));
  if(nregs / 32) {
    std::cout << " * 2^" << 32 * (nregs / 32);
  }
  std::cout << std::endl;
}

std::string d2b(std::string decimal) {
  std::string binary;
  while(decimal.size() > 32) {
    unsigned long s = std::stoul(decimal.substr(decimal.size() - 32));
    for(int i = 0; i < 32; i++) {
      if(s % 2) {
	binary += "1";
      }
      else {
	binary += "0";
      }
      s = s >> 1;
    }
    decimal = decimal.substr(0, decimal.size() - 32);
  }
  unsigned long s = std::stoul(decimal);
  for(int i = 0; i < 32; i++) {
    if(s % 2) {
      binary += "1";
    }
    else {
      binary += "0";
    }
    s = s >> 1;
  }
  return binary;
}

template <typename node> 
node initial_function(Bdd::BddMan<node> & bdd, node & init, std::string nzero, int npis, int nregs) {
  std::mt19937 rnd;
  node x = bdd.Const1();
  if(nzero[0] != '-') {
    nzero = d2b(nzero);
    std::string i;
    for(int j = 0; j < nzero.size(); j++) {
      i += "0";
    }
    while(1) {
      while(1) {
	node y = bdd.Const1();
	for(int j = 0; j < nregs; j++) {
	  if((int)rnd() > 0)
	    y = bdd.And(y, bdd.IthVar(npis+j));
	  else
	    y = bdd.And(y, bdd.Not(bdd.IthVar(npis+j)));
	}
	if(y == init) {
	  continue;
	}
	if(bdd.And(bdd.Not(x), y) != bdd.Const0()) {
	  continue;
	}
	x = bdd.And(x, bdd.Not(y));
	break;
      }
      int idx = 0;
      while(i[idx] != '0') {
	i[idx] = '0';
	idx++;
      }
      i[idx] = '1';
      if(i == nzero) {
	break;
      }
    }
  }
  else {
    x = init;
    nzero = nzero.substr(1);
    if(nzero == "1") {
      return x;
    }
    nzero = d2b(nzero);
    std::string i;
    for(int j = 0; j < nzero.size(); j++) {
      i += "0";
    }
    i[0] = '1';
    while(1) {
      while(1) {
	node y = bdd.Const1();
	for(int j = 0; j < nregs; j++) {
	  if((int)rnd() > 0)
	    y = bdd.And(y, bdd.IthVar(npis+j));
	  else
	    y = bdd.And(y, bdd.Not(bdd.IthVar(npis+j)));
	}
	if(bdd.And(x, y) != bdd.Const0()) {
	  continue;
	}
	x = bdd.Or(x, y);
	break;
      }
      int idx = 0;
      while(i[idx] != '0') {
	i[idx] = '0';
	idx++;
      }
      i[idx] = '1';
      if(i == nzero) {
	break;
      }
    }
  }
  return x;
}

template <typename node> 
void IIG(mockturtle::aig_network & aig_, Bdd::BddMan<node> & bdd, std::string initstr, std::string nzero, std::string filename) {
  int npis = aig_.num_pis();
  int nregs = aig_.num_registers();
  std::cout << "PI : " << npis << " , REG : " << nregs << std::endl;
  mockturtle::aig_network aig;
  CombNoPo(aig_, aig);

  node pis = bdd.Const1();
  for(int j = 0; j < npis; j++) {
    pis = bdd.And(pis, bdd.IthVar(j));
  }

  node init = bdd.Const1();
  for(int i = 0; i < nregs; i++) {
    if(initstr[i] == '0')
      init = bdd.And(init, bdd.Not(bdd.IthVar(npis+i)));
    else
      init = bdd.And(init, bdd.IthVar(npis+i));
  }

  auto t1 = std::chrono::system_clock::now();
  auto t2 = t1;
  
  std::cout << "init rnd func ";
  node x = initial_function(bdd, init, nzero, npis, nregs);
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;
  
  std::cout << "build latch   ";
  std::vector<node> vNodes = Aig2Bdd( aig, bdd );
  for(int i = 0; i < npis; i++) {
    vNodes.insert(vNodes.begin() + i, bdd.IthVar(i));
  }
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;

  std::cout << std::endl << "##### begin iig #####" << std::endl;
  auto start = t1;
  int itr = 0;
  while(1) {
    std::cout << "iteration " << itr++ << "   ";
    node y = bdd.VecCompose(x, vNodes);
    node z = bdd.Or(bdd.Not(x), y);
    if(z == bdd.Const1())
      break;
    node k = bdd.Univ(z, pis);
    x = bdd.And(x, k);
    if(bdd.And(x, init) == bdd.Const0()) {
      x = bdd.Const0();
      break;
    }
    t2 = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
    t1 = t2;
  }
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;

  std::cout << "total " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-start).count() / 1000.0 << " sec" << std::endl;
  
  if(x == bdd.Const0())
    std::cout << "fail ... function excluded initial state" << std::endl;
  else if(x == bdd.Const1())
    std::cout << "fail ... function is const 1" << std::endl;
  else {
    std::cout << "success" << std::endl;
    std::ofstream * f = NULL;
    if(!filename.empty()) {
      f = new std::ofstream(filename);
    }
    show_bdd(bdd, x, npis, nregs, f);
    if(f) {
      f->close();
    }
  }
}

template <typename node> 
void RIIG(mockturtle::aig_network & aig_, Bdd::BddMan<node> & bdd, std::string initstr, std::string nzero, std::string filename) {
  int npis = aig_.num_pis();
  int nregs = aig_.num_registers();
  std::cout << "PI : " << npis << " , REG : " << nregs << std::endl;
  mockturtle::aig_network aig;
  CombNoPo(aig_, aig);
  
  node cis = bdd.Const1();
  for(int j = 0; j < npis+nregs; j++) {
    cis = bdd.And(cis, bdd.IthVar(j));
  }
  
  node init = bdd.Const1();
  for(int i = 0; i < nregs; i++) {
    if(initstr[i] == '0')
      init = bdd.And(init, bdd.Not(bdd.IthVar(npis+i)));
    else
      init = bdd.And(init, bdd.IthVar(npis+i));
  }
  
  auto t1 = std::chrono::system_clock::now();
  auto t2 = t1;
  
  std::cout << "init rnd func ";
  node x = initial_function(bdd, init, nzero, npis, nregs);
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;

  std::cout << "build latch   ";
  std::vector<node> vNodes = Aig2Bdd( aig, bdd );
  
  node tra = bdd.Const0();
  for(int i = 0; i < nregs; i++) {
    tra = bdd.Or(tra, bdd.Ite(vNodes[i], bdd.IthVar(npis+nregs+i), bdd.Not(bdd.IthVar(npis+nregs+i))));
  }
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;
  
  for(int i = 0; i < npis; i++) {
    vNodes.insert(vNodes.begin() + i, bdd.IthVar(i));
  }
  for(int i = 0; i < nregs; i++) {
    vNodes.push_back(bdd.IthVar(npis+nregs+i));
  }

  std::vector<node> shift;
  for(int i = 0; i < npis+nregs; i++) {
    shift.push_back(bdd.IthVar(i));
  }
  for(int i = 0; i < nregs; i++) {
    shift.push_back(bdd.IthVar(npis+i));
  }

  std::cout << std::endl << "##### begin iig #####" << std::endl;
  auto start = t1;
  int itr = 0;
  while(1) {
    std::cout << "iteration " << itr++ << "   ";
    node y = bdd.VecCompose(x, vNodes);
    node z = bdd.Or(bdd.Not(x), y);
    if(z == bdd.Const1())
      break;
    node ns = bdd.And(tra, bdd.Not(z));
    ns = bdd.Exist(ns, cis);
    ns = bdd.VecCompose(ns, shift);
    x = bdd.Or(x, ns);
    if(x == bdd.Const1()) {
      break;
    }
    t2 = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
    t1 = t2;
  }
  t2 = std::chrono::system_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() / 1000.0 << " sec" << std::endl;
  t1 = t2;

  std::cout << "total " << std::chrono::duration_cast<std::chrono::milliseconds>(t1-start).count() / 1000.0 << " sec" << std::endl;  
  
  if(x == bdd.Const1())
    std::cout << "fail ... function became const 1" << std::endl;
  else {
    std::cout << "success" << std::endl;
    std::ofstream * f = NULL;
    if(!filename.empty()) {
      f = new std::ofstream(filename);
    }
    show_bdd(bdd, x, npis, nregs, f);
    if(f) {
      f->close();
    }
  }
}
#endif
