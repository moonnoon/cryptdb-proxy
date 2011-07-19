/*
 * Binary.cpp
 *
 *  Created on: May 18, 2011
 *      Author: raluca
 */

#include "Binary.h"
#include <list>
#include <iostream>


Binary::Binary() {

}

Binary::Binary(const Binary & b) {
	len = b.len;
	content = new unsigned char[b.len];
	memcpy(content, b.content, b.len);
}

Binary::~Binary() {
}

void Binary::Free() {
	free(content);
}

Binary::Binary(unsigned int leng) {
	this->len = leng;
	content = new unsigned char[leng];
}

//allocates len bytes and copies
Binary::Binary(unsigned int leng, unsigned char * val) {
	this->len = leng;
	content = new unsigned char[leng];

	memcpy(content, val, leng);
}

Binary Binary::operator+(const Binary & b) const {
	Binary c;

	c.len = this->len + b.len;
	c.content = new unsigned char[this->len+b.len];

	memcpy(c.content, this->content, this->len);
	memcpy(c.content+this->len, b.content, b.len);

	return c;
}

Binary Binary::operator ^ (const Binary & b) const {


	unsigned int minlen = min(len, b.len);
	unsigned int maxlen = max(len, b.len);

	Binary result(maxlen);

	for (unsigned int i = 0; i < minlen; i++) {
		result.content[i] = b.content[i] xor content[i];
	}

	if (b.len > len) {
		for (unsigned int i = minlen; i < b.len; i++) {
			result.content[i] = b.content[i];
		}
	} else {
		for (unsigned int i = minlen; i < len; i++) {
			result.content[i] = content[i];
		}
	}

	return result;

}

bool Binary::operator == (const Binary & b) const {
	if (len != b.len) {
		return false;
	}

	for (unsigned int i = 0; i < len ; i++) {
		if (b.content[i] != content[i]) {
			return false;
		}
	}

	return true;
}


Binary Binary::subbinary(unsigned int pos, unsigned int no) const {
	Binary res;

	if (this->len < pos + no) {
		string msg =  "subbinary problem: len < pos + no";
		cerr << "len is " << len << " pos is " << pos << " no is " << no << "\n";
		cerr << msg << "\n";
		throw (void *) msg.c_str();
	}
	res.len = no;
	res.content = new unsigned char[no];

	memcpy(res.content, this->content + pos, no);

	return res;
}



Binary Binary::toBinary(unsigned long val, int no_bytes) {
	list<int> reslist;

	while (val > 0) {
		reslist.push_back(val % 256);
		val = val / 256;
	}

	unsigned int size;
	if (no_bytes > 0) {
		size = no_bytes;
	} else {
		size = reslist.size();
	}

	Binary res(size);

	unsigned int index = 0;

	for (list<int>::iterator it = reslist.begin(); it != reslist.end(); it++) {
		res.content[index] = (unsigned char)*it;
		index++;
	}

	for (unsigned int i = reslist.size(); i < size; i++) {
		res.content[i] = 0;
	}

	return res;
}

Binary Binary::toBinary(string val) {
	Binary res(val.length());
	memcpy(res.content, val.c_str(), res.len);
	return res;
}

unsigned int Binary::toUInt() {
	unsigned int res = 0;
	for (int i = len-1; i >= 0; i--) {
		res = res * 10 + content[i];
	}

	return res;
}

// ?? Why do I have to forward declare the function?
string valueToString(unsigned int x);

string valueToString(unsigned int x) {

	if (x == 0) {
		return "0";
	}

	string res = "";
	while (x > 0) {
		char c = (char)('0' + x % 10);
		res = c + res;
		x = x / 10;
	}

	return res;

}


string Binary::toString() const {
	string res = "";

	for (unsigned i = 0; i < len; i++) {
		res += valueToString(content[i]) + " ";
	}

	return res;
}

list<Binary> * Binary::split(unsigned int plen) const {

	if (len % plen != 0) {
		cerr <<  "split receives invalid input \n";
		throw "split receives invalid input";
	}

	unsigned int num = len / plen;
	list<Binary> * res = new list<Binary>();

	for (unsigned int i = 0; i < num; i++) {
		res->push_back(subbinary(i*plen, plen));
	}

	return res;
}



Binary::Binary(const list<Binary> & ciphs) {
	len = 0;

	for (list<Binary>::const_iterator it = ciphs.begin(); it != ciphs.end(); it++) {
		len += it->len;
	}

	content = new unsigned char[len];

	unsigned int index = 0;
	for (list<Binary>::const_iterator it = ciphs.begin(); it != ciphs.end(); it++) {
			memcpy(content+index, it->content, it->len);
			index = index + it->len;
	}


}
